// Copyright (c) 2013 Blowmorph Team

#include "server/server.h"

#include <cstdio>
#include <cstring>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/error.h"
#include "base/id_manager.h"
#include "base/macros.h"

#include "base/pstdint.h"
#include "base/time.h"

#include "net/enet.h"
#include "net/utils.h"

#include "engine/config.h"
#include "engine/protocol.h"

#include "server/client_manager.h"
#include "server/controller.h"
#include "server/entity.h"

#include "server/activator.h"
#include "server/critter.h"
#include "server/kit.h"
#include "server/projectile.h"
#include "server/player.h"
#include "server/wall.h"

namespace bm {

Server::Server() : controller_(),
  state_(STATE_FINALIZED), host_(NULL), event_(NULL) { }

Server::~Server() {
  if (state_ == STATE_INITIALIZED) {
    Finalize();
  }
}

bool Server::Initialize() {
  CHECK(state_ == STATE_FINALIZED);

  if (!Config::GetInstance()->Initialize()) {
    return false;
  }

  const Config::ServerConfig& config =
    Config::GetInstance()->GetServerConfig();

  uint32_t update_rate = config.tick_rate;
  update_timeout_ = 1000 / update_rate;
  last_update_ = 0;

  uint32_t broadcast_rate = config.broadcast_rate;
  broadcast_timeout_ = 1000 / broadcast_rate;
  last_broadcast_ = 0;

  host_ = NULL;
  event_ = NULL;

  if (!controller_.GetWorld()->LoadMap(config.map)) {
    return false;
  }

  if (!enet_.Initialize()) {
    return false;
  }

  std::auto_ptr<ServerHost> host(enet_.CreateServerHost(config.port));
  if (host.get() == NULL) {
    return false;
  }

  std::auto_ptr<Event> event(enet_.CreateEvent());
  if (event.get() == NULL) {
    return false;
  }

  host_ = host.release();
  event_ = event.release();

  state_ = STATE_INITIALIZED;
  return true;
}

void Server::Finalize() {
  CHECK(state_ == STATE_INITIALIZED);
  if (event_ != NULL) {
    delete event_;
    event_ = NULL;
  }
  if (host_ != NULL) {
    delete host_;
    host_ = NULL;
  }
  state_ = STATE_FINALIZED;
}

bool Server::Tick() {
  CHECK(state_ == STATE_INITIALIZED);

  int64_t current_time = Timestamp();
  if (current_time - last_broadcast_ >= broadcast_timeout_) {
    if (!BroadcastDynamicEntities()) {
      return false;
    }
    if (!BroadcastStaticEntities()) {
      return false;
    }
    if (!BroadcastGameEvents()) {
      return false;
    }
    last_broadcast_ = current_time;
  }

  current_time = Timestamp();
  if (current_time - last_update_ >= update_timeout_) {
    controller_.Update(current_time, current_time - last_update_);
    last_update_ = current_time;
  }

  if (!PumpEvents()) {
    return false;
  }

  int64_t next_broadcast = last_broadcast_ + broadcast_timeout_;
  int64_t next_update = last_update_ + update_timeout_;
  int64_t sleep_until = std::min(next_broadcast, next_update);
  current_time = Timestamp();

  if (current_time <= sleep_until) {
    uint32_t timeout = static_cast<uint32_t>(sleep_until - current_time);
    bool rv = host_->Service(NULL, timeout);
    if (rv == false) {
      return false;
    }
  } else {
    printf("Can't keep up, %ld ms behind!\n", current_time - sleep_until);
  }

  return true;
}

bool Server::BroadcastDynamicEntities() {
  for (auto itr : *controller_.GetWorld()->GetDynamicEntities()) {
    ServerEntity* entity = static_cast<ServerEntity*>(itr.second);
    bool rv = BroadcastEntityRelatedMessage(
        Packet::TYPE_ENTITY_UPDATED, entity);
    if (rv == false) {
      return false;
    }
  }
  return true;
}

bool Server::BroadcastStaticEntities(bool force) {
  for (auto itr : *controller_.GetWorld()->GetStaticEntities()) {
    ServerEntity* entity = static_cast<ServerEntity*>(itr.second);
    if (force || entity->IsUpdated()) {
      bool rv = BroadcastEntityRelatedMessage(
          Packet::TYPE_ENTITY_UPDATED, entity);
      if (rv == false) {
        return false;
      }
      entity->SetUpdatedFlag(false);
    }
  }

  return true;
}

bool Server::BroadcastGameEvents() {
  std::vector<GameEvent> *events = controller_.GetGameEvents();
  std::vector<GameEvent>::iterator it;
  for (it = events->begin(); it != events->end(); ++it) {
    bool rv = BroadcastPacket(host_, Packet::TYPE_GAME_EVENT, *it, true);
    if (rv == false) {
      return false;
    }
  }
  events->clear();
  return true;
}

bool Server::PumpEvents() {
  do {
    // TODO(xairy): timeout.
    if (host_->Service(event_, 0) == false) {
      return false;
    }

    switch (event_->GetType()) {
      case Event::TYPE_CONNECT: {
        OnConnect();
        break;
      }

      case Event::TYPE_RECEIVE: {
        if (!OnReceive()) {
          return false;
        }
        break;
      }

      case Event::TYPE_DISCONNECT: {
        if (!OnDisconnect()) {
          return false;
        }
        break;
      }

      case Event::TYPE_NONE:
        break;
    }
  } while (event_->GetType() != Event::TYPE_NONE);

  return true;
}

void Server::OnConnect() {
  CHECK(event_->GetType() == Event::TYPE_CONNECT);

  uint32_t client_id = id_manager_.NewId();
  event_->GetPeer()->SetData(reinterpret_cast<void*>(client_id));

  printf("#%u: Client from %s:%u is trying to connect.\n", client_id,
    event_->GetPeer()->GetIp().c_str(), event_->GetPeer()->GetPort());

  // Client should send 'TYPE_LOGIN' packet now.
}

bool Server::OnDisconnect() {
  CHECK(event_->GetType() == Event::TYPE_DISCONNECT);

  void* peer_data = event_->GetPeer()->GetData();
  // So complicated to make it work under both x32 and x64.
  uint32_t id = static_cast<uint32_t>(reinterpret_cast<size_t>(peer_data));
  Client* client = client_manager_.GetClient(id);

  controller_.OnPlayerDisconnected(client->entity);

  client_manager_.DeleteClient(id, true);

  printf("#%u: Client from %s:%u disconnected.\n", id,
    event_->GetPeer()->GetIp().c_str(), event_->GetPeer()->GetPort());

  return true;
}

bool Server::OnReceive() {
  CHECK(event_->GetType() == Event::TYPE_RECEIVE);

  void* peer_data = event_->GetPeer()->GetData();
  // So complicated to make it work under both x32 and x64.
  uint32_t id = static_cast<uint32_t>(reinterpret_cast<size_t>(peer_data));

  std::vector<char> message;
  event_->GetData(&message);

  Packet::Type packet_type;
  bool rv = ExtractPacketType(message, &packet_type);
  if (rv == false) {
    printf("#%u: Incorrect message format [5], client dropped.\n", id);
    client_manager_.DisconnectClient(id);
    return true;
  }

  if (packet_type == Packet::TYPE_LOGIN) {
    if (!OnLogin(id)) {
      return false;
    }
    return true;
  }

  Client* client = client_manager_.GetClient(id);

  switch (packet_type) {
    case Packet::TYPE_SYNC_TIME_REQUEST: {
      TimeSyncData sync_data;
      rv = ExtractPacketData<Packet::Type, TimeSyncData>(message, &sync_data);
      if (rv == false) {
        printf("#%u: Incorrect message format [0], client dropped.\n", id);
        client_manager_.DisconnectClient(id);
        return true;
      }

      sync_data.server_time = Timestamp();
      packet_type = Packet::TYPE_SYNC_TIME_RESPONSE;

      rv = SendPacket(client->peer, packet_type, sync_data, true);
      if (rv == false) {
        return false;
      }

      host_->Flush();
    } break;

    case Packet::TYPE_CLIENT_STATUS: {
      if (!OnClientStatus(id)) {
        return false;
      }
    } break;

    case Packet::TYPE_KEYBOARD_EVENT: {
      KeyboardEvent event;
      rv = ExtractPacketData<Packet::Type, KeyboardEvent>(message, &event);
      if (rv == false) {
        printf("#%u: Incorrect message format [1], client dropped.\n", id);
        client_manager_.DisconnectClient(id);
        return true;
      }
      controller_.OnKeyboardEvent(client->entity, event);
    } break;

    case Packet::TYPE_MOUSE_EVENT: {
      MouseEvent event;
      rv = ExtractPacketData<Packet::Type, MouseEvent>(message, &event);
      if (rv == false) {
        printf("#%u: Incorrect message format [2], client dropped.\n", id);
        client_manager_.DisconnectClient(id);
        return true;
      }
      controller_.OnMouseEvent(client->entity, event);
    } break;

    case Packet::TYPE_PLAYER_ACTION: {
      PlayerAction action;
      rv = ExtractPacketData<Packet::Type, PlayerAction>(message, &action);
      if (rv == false) {
        printf("#%u: Incorrect message format [3], client dropped.\n", id);
        client_manager_.DisconnectClient(id);
        return true;
      }
      controller_.OnPlayerAction(client->entity, action);
    } break;


    default: {
      printf("#%u: Incorrect message format [4], client dropped.\n", id);
      client_manager_.DisconnectClient(id);
      return true;
    } break;
  }

  return true;
}

bool Server::OnLogin(uint32_t client_id) {
  Peer* peer = event_->GetPeer();
  CHECK(peer != NULL);

  // Receive login data.

  std::vector<char> message;
  event_->GetData(&message);

  LoginData login_data;
  bool rv = ExtractPacketData<Packet::Type, LoginData>(message, &login_data);
  if (rv == false) {
    printf("#%u: Incorrect message format [4], client dropped.\n", client_id);
    return true;
  }

  printf("#%u: Login data has been received.\n", client_id);

  // Create player.

  Player* player = controller_.OnPlayerConnected();

  login_data.login[LoginData::MAX_LOGIN_LENGTH] = '\0';
  std::string login(&login_data.login[0]);
  Client* client = new Client(peer, player, login);
  CHECK(client != NULL);
  client_manager_.AddClient(client_id, client);

  if (!SendClientOptions(client)) {
    return false;
  }

  printf("#%u: Client options has been sent.\n", client_id);

  // Broadcast the new player info.

  // The new player may not receive this info, as he will be
  // synchronizing time and ignoring everything else.

  if (!BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_APPEARED,
      client->entity)) {
    return false;
  }

  PlayerInfo player_info;
  player_info.id = player->GetId();
  std::copy(login.c_str(), login.c_str() + login.size() + 1,
      &player_info.login[0]);
  rv = BroadcastPacket(host_, Packet::TYPE_PLAYER_INFO, player_info, true);
  if (rv == false) {
    return false;
  }

  printf("#%u: Client from %s:%u connected.\n", client_id,
    event_->GetPeer()->GetIp().c_str(), event_->GetPeer()->GetPort());

  return true;
}

bool Server::SendClientOptions(Client* client) {
  ClientOptions options;
  options.id = client->entity->GetId();
  options.speed = client->entity->GetSpeed();
  options.x = client->entity->GetPosition().x;
  options.y = client->entity->GetPosition().y;
  options.max_health = client->entity->GetMaxHealth();
  options.energy_capacity = client->entity->GetEnergyCapacity();

  Packet::Type packet_type = Packet::TYPE_CLIENT_OPTIONS;

  bool rv = SendPacket(client->peer, packet_type, options, true);
  if (rv == false) {
    return false;
  }

  return true;
}

bool Server::OnClientStatus(uint32_t client_id) {
  // Send to the new player all players' info.

  PlayerInfo player_info;
  Client* client = client_manager_.GetClient(client_id);

  for (auto i : *client_manager_.GetClients()) {
    player_info.id = i.second->entity->GetId();
    std::string& login = i.second->login;
    std::copy(login.c_str(), login.c_str() + login.size() + 1,
        &player_info.login[0]);
    bool rv = SendPacket(client->peer, Packet::TYPE_PLAYER_INFO,
        player_info, true);
    if (rv == false) {
      return false;
    }
  }

  // And all the static entities.

  if (!BroadcastStaticEntities(true)) {
    return false;
  }

  return true;
}

bool Server::BroadcastEntityRelatedMessage(Packet::Type packet_type,
    ServerEntity* entity) {
  CHECK(packet_type == Packet::TYPE_ENTITY_APPEARED ||
    packet_type == Packet::TYPE_ENTITY_UPDATED);

  EntitySnapshot snapshot;
  entity->GetSnapshot(Timestamp(), &snapshot);

  bool rv = BroadcastPacket(host_, packet_type, snapshot, true);
  if (rv == false) {
    return false;
  }

  return true;
}

}  // namespace bm
