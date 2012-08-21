#ifndef BLOWMORPH_SERVER_SERVER_HPP_
#define BLOWMORPH_SERVER_SERVER_HPP_

#include <cstdio>
#include <cstring>

#include <memory>

#include <enet-wrapper/enet.hpp>

#include <base/error.hpp>
#include <base/macros.hpp>
#include <base/protocol.hpp>
#include <base/pstdint.hpp>
#include <base/timer.hpp>

#include "client_manager.hpp"
#include "entity.hpp"
#include "id_manager.hpp"
#include "vector.hpp"
#include "shape.hpp"
#include "settings_manager.hpp"
#include "world_manager.hpp"

#include "bullet.hpp"
#include "dummy.hpp"
#include "player.hpp"
#include "wall.hpp"
#include "station.hpp"

namespace {

bm::uint32_t min(bm::uint32_t a, bm::uint32_t b) {
  return a < b ? a : b;
}

} // anonymous namespace

namespace bm {

class Server {
public:
  Server() : _world_manager(&_id_manager, &_settings) { }

  ~Server() {
    _Destroy();
  }

  bool Execute() {
    if(!_Initialize()) {
      return false;
    }

    printf("Server started.\n");

    _is_running = true;

    while(_is_running) {
      if(!_Tick()) {
        return false;
      }
    }

    printf("Server finished.\n");

    return true;
  }

private:
  bool _Initialize() {
    if(!_settings.Load("data/server.xml")) {
      return false;
    }

    _is_running = false;
    _server_port = _settings.GetValue("server.port", 4242);

    _broadcast_rate = _settings.GetValue("server.broadcast_rate", 20);
    _broadcast_time = 1000 / _broadcast_rate;
    _last_broadcast = 0;

    _update_rate = _settings.GetValue("server.update_rate", 100);
    _update_time = 1000 / _update_rate;
    _last_update = 0;

    _latency_limit = _settings.GetValue("server.latency_limit", 100);

    _host = NULL;
    _event = NULL;

    _map_file = _settings.GetValue("server.map", std::string("data/map.xml"));

    if(!_world_manager.LoadMap(_map_file)) {
      return false;
    }

    if(!_enet.Initialize()) {
      return false;
    }

    std::auto_ptr<ServerHost> host(_enet.CreateServerHost(_server_port));
    if(host.get() == NULL) {
      return false;
    }

    std::auto_ptr<Event> event(_enet.CreateEvent());
    if(event.get() == NULL) {
      return false;
    }

    _host = host.release();
    _event = event.release();

    return true;
  }

  void _Destroy() {
    if(_event != NULL) {
      delete _event;
    }
    if(_host != NULL) {
      delete _host;
    }
  }

  bool _Tick() {
    if(_timer.GetTime() - _last_broadcast >= _broadcast_time) {
      _last_broadcast = _timer.GetTime();
      if(!_BroadcastDynamicEntities()) {
        return false;
      }
      if(!_BroadcastStaticEntities()) {
        return false;
      }
    }

    if(_timer.GetTime() - _last_update >= _update_time) {
      _last_update = _timer.GetTime();
      if(!_UpdateWorld()) {
        return false;
      }
    }

    if(!_PumpEvents()) {
      return false;
    }

    uint32_t next_broadcast = _last_broadcast + _broadcast_time;
    uint32_t next_update = _last_update + _update_time;
    uint32_t sleep_until = min(next_broadcast, next_update);
    uint32_t current_time = _timer.GetTime();

    if(current_time <= sleep_until) {
      bool rv = _host->Service(NULL, sleep_until - current_time);
      if(rv == false) {
        return false;
      }
    } else {
      printf("Can't keep up, %u ms behind!\n", current_time - sleep_until);
    }

    return true;
  }

  bool _UpdateWorld() {
    // Temporary.
    static int counter = 0;
    if(counter == 300) {
      float x = -500.0f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 1000.0f;
      float y = -500.0f + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 1000.0f;
      _world_manager.CreateDummy(Vector2(x, y), _timer.GetTime());
      counter = 0;
      //printf("Dummy spawned at (%.2f, %.2f)\n", x, y);
    }
    counter++;

    _world_manager.UpdateEntities(_timer.GetTime());

    _world_manager.CollideEntities();

    _world_manager.DestroyOutlyingEntities();

    if(!_DeleteDestroyedEntities()) {
      return false;
    }

    return true;
  }

  bool _DeleteDestroyedEntities() {
    std::vector<uint32_t> destroyed_entities;
    _world_manager.GetDestroyedEntities(&destroyed_entities);

    size_t size = destroyed_entities.size();
    for(size_t i = 0; i < size; i++) {
      uint32_t id = destroyed_entities[i];
      bool rv = _BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_DISAPPEARED,
        _world_manager.GetEntity(id));
      if(rv == false) {
        return false;
      }
    }

    _world_manager.DeleteEntities(destroyed_entities, true);

    return true;
  }

  bool _BroadcastStaticEntities(bool force = false) {
    std::map<uint32_t, Entity*>* _entities =
      _entities = _world_manager.GetStaticEntities();
    std::map<uint32_t, Entity*>::iterator itr, end;
    end = _entities->end();
    for(itr = _entities->begin(); itr != end; ++itr) {
      Entity* entity = itr->second;
      if(force || entity->IsUpdated()) {
        bool rv = _BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_UPDATED, entity);
        if(rv == false) {
          return false;
        }
        // XXX[18.08.2012 xairy]: hack.
        if(entity->GetType() == "Wall") {
          entity->SetUpdatedFlag(false);
        }
      }
    }

    return true;
  }

  bool _BroadcastDynamicEntities() {
    std::map<uint32_t, Entity*>* _entities =
      _entities = _world_manager.GetDynamicEntities();
    std::map<uint32_t, Entity*>::iterator itr, end;
    end = _entities->end();
    for(itr = _entities->begin(); itr != end; ++itr) {
      Entity* entity = itr->second;
      bool rv = _BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_UPDATED, entity);
      if(rv == false) {
        return false;
      }
    }
    return true;
  }

  bool _PumpEvents() {
    do {
      if(_host->Service(_event, 0) == false) {
        return false;
      }
      
      switch(_event->GetType()) {
        case Event::EVENT_CONNECT: {
          if(!_OnConnect()) {
            return false;
          }
          break;
        }

        case Event::EVENT_RECEIVE: {
          if(!_OnReceive()) {
            return false;
          }
          break;
        }

        case Event::EVENT_DISCONNECT: {
          if(!_OnDisconnect()) {
            return false;
          }
          break;
        }
      }
    } while(_event->GetType() != Event::EVENT_NONE);

    return true;
  }

  bool _OnConnect() {
    CHECK(_event->GetType() == Event::EVENT_CONNECT);

    printf("Client from %s:%u is trying to connect.\n",
      _event->GetPeerIp().c_str(), _event->GetPeerPort());

    std::auto_ptr<Peer> peer(_event->GetPeer());
    if(peer.get() == NULL) {
      return false;
    }

    uint32_t client_id = _id_manager.NewId();
    peer->SetData(reinterpret_cast<void*>(client_id));

    Player* player = Player::Create(
      &_world_manager,
      client_id,
      Vector2(0.0f, 0.0f)
    );
    if(player == NULL) {
      Error::Set(Error::TYPE_MEMORY);
      return false;
    }
    player->Respawn();
    Client* client = new Client(peer.release(), player);
    if(client == NULL) {
      Error::Set(Error::TYPE_MEMORY);
      return false;
    }
    _client_manager.AddClient(client_id, client);
    _world_manager.AddEntity(client_id, player);

    printf("Client from %s:%u connected. Id #%u assigned.\n",
      _event->GetPeerIp().c_str(), _event->GetPeerPort(), client_id);

    if(!_SendClientOptions(client)) {
      return false;
    }

    if(!_BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_APPEARED,
        client->entity)) {
      return false;
    }

    return true;
  }

  bool _SendClientOptions(Client* client) {
    ClientOptions options;
    options.id = client->entity->GetId();
    options.speed = client->entity->GetSpeed();
    options.x = client->entity->GetPosition().x;
    options.y = client->entity->GetPosition().y;
    options.max_health = client->entity->GetMaxHealth();
    options.blow_capacity = client->entity->GetBlowCapacity();
    options.morph_capacity = client->entity->GetMorphCapacity();

    Packet::Type packet_type = Packet::TYPE_CLIENT_OPTIONS;

    bool rv = _SendPacket(client->peer, packet_type, options, true);
    if(rv == false) {
      return false;
    }

    printf("#%u: Client options has been sent.\n", client->entity->GetId());

    return true;
  }

  bool _BroadcastEntityRelatedMessage(Packet::Type packet_type, Entity* entity) {
    CHECK(packet_type == Packet::TYPE_ENTITY_APPEARED ||
      packet_type == Packet::TYPE_ENTITY_DISAPPEARED ||
      packet_type == Packet::TYPE_ENTITY_UPDATED);

    EntitySnapshot snapshot;
    entity->GetSnapshot(_timer.GetTime(), &snapshot);

    bool rv = _BroadcastPacket(packet_type, snapshot, true);
    if(rv == false) {
      return false;
    }

    //if(packet_type == TYPE_ENTITY_APPEARED) {
    //  printf("Entity %u appeared.\n", entity->GetId());
    //}
    //if(packet_type == TYPE_ENTITY_DISAPPEARED) {
    //  printf("Entity %u disappeared.\n", entity->GetId());
    //}

    return true;
  }

  bool _OnDisconnect() {
    CHECK(_event->GetType() == Event::EVENT_DISCONNECT);

    void* peer_data = _event->GetPeerData();
    // So complicated to make it work under x64.
    uint32_t id = static_cast<uint32_t>(reinterpret_cast<size_t>(peer_data));
    Client* client = _client_manager.GetClient(id);

    bool rv = _BroadcastEntityRelatedMessage(Packet::TYPE_ENTITY_DISAPPEARED,
      client->entity);
    if(rv == false) {
      return false;
    }
    
    client->entity->Destroy();
    _client_manager.DeleteClient(id, true);

    printf("#%u from %s:%u disconnected.\n", id,
      _event->GetPeerIp().c_str(), _event->GetPeerPort());

    return true;
  }

  bool _OnReceive() {
    CHECK(_event->GetType() == Event::EVENT_RECEIVE);

    void* peer_data = _event->GetPeerData();
    // So complicated to make it work under x64.
    uint32_t id = static_cast<uint32_t>(reinterpret_cast<size_t>(peer_data));
    Client* client = _client_manager.GetClient(id);

    std::vector<char> message;
    _event->GetData(&message);

    Packet::Type packet_type;
    bool rv = _ExtractPacketType(message, &packet_type);
    if(rv == false) {
      printf("#%u: Client dropped due to incorrect message format.0\n", id);
      _client_manager.DisconnectClient(id);
      return true;
    }

    if(packet_type == Packet::TYPE_KEYBOARD_EVENT) {
      if(message.size() != sizeof(Packet::Type) + sizeof(KeyboardEvent)) {
        printf("#%u: Client dropped due to incorrect message format.1\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      KeyboardEvent keyboard_event;
      rv = _ExtractData(message, &keyboard_event);
      if(rv == false) {
        printf("#%u: Client dropped due to incorrect message format.2\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      client->entity->OnKeyboardEvent(keyboard_event);
    } else if(packet_type == Packet::TYPE_MOUSE_EVENT) {
      if(message.size() != sizeof(Packet::Type) + sizeof(MouseEvent)) {
        printf("#%u: Client dropped due to incorrect message format.3\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      MouseEvent mouse_event;
      rv = _ExtractData(message, &mouse_event);
      if(rv == false) {
        printf("#%u: Client dropped due to incorrect message format.4\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      if(!client->entity->OnMouseEvent(mouse_event, _timer.GetTime())) {
        return false;
      }
    } else if(packet_type == Packet::TYPE_SYNC_TIME_REQUEST) {
      if(message.size() != sizeof(Packet::Type) + sizeof(TimeSyncData)) {
        printf("#%u: Client dropped due to incorrect message format.5\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      TimeSyncData sync_data;
      rv = _ExtractData(message, &sync_data);
      if(rv == false) {
        printf("#%u: Client dropped due to incorrect message format.6\n", id);
        _client_manager.DisconnectClient(id);
        return true;
      }

      sync_data.server_time = _timer.GetTime();
      packet_type = Packet::TYPE_SYNC_TIME_RESPONSE;

      rv = _SendPacket(client->peer, packet_type, sync_data, true);
      if(rv == false) {
        return false;
      }

      _host->Flush();

      printf("#%u: Time syncronized: client: %u, server: %u.\n",
        client->entity->GetId(), sync_data.client_time, sync_data.server_time);

      //XXX[14.08.2012 xairy]: hack?
      if(!_BroadcastStaticEntities(true)) {
        return false;
      }
    } else {
      printf("#%u: Client dropped due to incorrect message format.7\n", id);
      _client_manager.DisconnectClient(id);
      return true;
    }

    return true;
  }

  template<class T>
  bool _BroadcastPacket(Packet::Type packet_type, const T& data, bool reliable) {
    std::vector<char> message;
    message.insert(message.end(), reinterpret_cast<const char*>(&packet_type),
      reinterpret_cast<const char*>(&packet_type) + sizeof(packet_type));
    message.insert(message.end(), reinterpret_cast<const char*>(&data),
      reinterpret_cast<const char*>(&data) + sizeof(T));

    bool rv = _host->Broadcast(&message[0], message.size(), reliable);
    if(rv == false) {
      return false;
    }
    return true;
  }

  template<class T>
  bool _SendPacket(Peer* peer, Packet::Type type, const T& data, bool reliable) {
    std::vector<char> message;
    message.insert(message.end(), reinterpret_cast<const char*>(&type),
      reinterpret_cast<const char*>(&type) + sizeof(type));
    message.insert(message.end(), reinterpret_cast<const char*>(&data),
      reinterpret_cast<const char*>(&data) + sizeof(T));

    bool rv = peer->Send(&message[0], message.size(), reliable);
    if(rv == false) {
      return false;
    }
    return true;
  }

  // Returns 'false' when message format is incorrect.
  bool _ExtractPacketType(const std::vector<char>& message, Packet::Type* type) {
    if(message.size() < sizeof(Packet::Type)) {
      return false;
    }
    memcpy(type, &message[0], sizeof(Packet::Type));
    return true;
  }

  // Returns 'false' when message format is incorrect.
  template<class T>
  bool _ExtractData(const std::vector<char>& message, T* data) {
    if(message.size() != sizeof(Packet::Type) + sizeof(T)) {
      return false;
    }
    memcpy(data, &message[0] + sizeof(Packet::Type), sizeof(T));
    return true;
  }

  bool _is_running;
  uint16_t _server_port;

  uint32_t _broadcast_rate;
  uint32_t _broadcast_time;
  uint32_t _last_broadcast;

  uint32_t _update_rate;
  uint32_t _update_time;
  uint32_t _last_update;

  uint32_t _latency_limit;

  std::string _map_file;

  Enet _enet;
  ServerHost* _host;
  Event* _event;

  IdManager _id_manager;
  WorldManager _world_manager;
  ClientManager _client_manager;
  Timer _timer;

  SettingsManager _settings;
};

} // namespace bm

#endif // BLOWMORPH_SERVER_SERVER_HPP_
