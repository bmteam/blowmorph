// Copyright (c) 2013 Blowmorph Team

#ifndef BASE_PROTOCOL_H_
#define BASE_PROTOCOL_H_

#include "base/macros.h"
#include "base/pstdint.h"

namespace bm {

class Packet {
 public:
  enum Type {
    TYPE_UNKNOWN = 0,

    // C -> S. Followed by 'LoginData'.
    TYPE_LOGIN,

    // S -> C. Followed by 'ClientOptions'.
    TYPE_CLIENT_OPTIONS,

    // C -> S. Followed by 'ClientStatus'.
    TYPE_CLIENT_STATUS,

    // C -> S. Followed by 'TimeSyncData' filled with client time.
    TYPE_SYNC_TIME_REQUEST,
    // S -> C. Followed by 'TimeSyncData' filled with client and server time.
    TYPE_SYNC_TIME_RESPONSE,

    // S -> C. Followed by 'PlayerInfo'.
    TYPE_PLAYER_INFO,

    // S -> C. Followed by 'EntitySnapshot' with the entity description.
    TYPE_ENTITY_APPEARED,
    TYPE_ENTITY_UPDATED,
    TYPE_ENTITY_DISAPPEARED,

    // C -> S. Followed by 'KeyboardEvent'.
    TYPE_KEYBOARD_EVENT,
    // C -> S. Followed by 'MouseEvent'.
    TYPE_MOUSE_EVENT,

    TYPE_MAX_VALUE
  };

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Packet);
};

struct LoginData {
  static const size_t MAX_LOGIN_LENGTH = 31;

  char login[MAX_LOGIN_LENGTH + 1];
};

struct ClientOptions {
  uint32_t id;
  float32_t speed;
  float32_t x, y;
  int32_t max_health;
  int32_t blow_capacity;
  int32_t morph_capacity;
};

struct TimeSyncData {
  int64_t client_time;
  int64_t server_time;
};

struct ClientStatus {
  enum Status {
    STATUS_SYNCHRONIZED
  };

  Status status;
};

struct PlayerInfo {
  uint32_t id;
  char login[LoginData::MAX_LOGIN_LENGTH + 1];
};

// type == EntitySnapshot::ENTITY_TYPE_PLAYER:
//   data[0] - health
//   data[1] - blow charge
//   data[2] - morph charge
//   data[3] - score
// type == EntitySnapshot::ENTITY_TYPE_WALL:
//    data[0] - wall type
// type == EntitySnapshot::ENTITY_TYPE_STATION:
//    data[0] - station type
struct EntitySnapshot {
  enum EntityType {
    ENTITY_TYPE_UNKNOWN = 0,

    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_BULLET,
    ENTITY_TYPE_WALL,
    ENTITY_TYPE_DUMMY,
    ENTITY_TYPE_STATION,
    ENTITY_TYPE_EXPLOSION,

    ENTITY_TYPE_MAX_VALUE
  };

  enum WallType {
    WALL_TYPE_UNKNOWN,

    WALL_TYPE_ORDINARY,
    WALL_TYPE_UNBREAKABLE,
    WALL_TYPE_MORPHED,

    WALL_TYPE_MAX_VALUE
  };

  enum StationType {
    STATION_TYPE_UNKNOWN,

    STATION_TYPE_HEALTH,
    STATION_TYPE_BLOW,
    STATION_TYPE_MORPH,
    STATION_TYPE_COMPOSITE,

    STATION_TYPE_MAX_VALUE
  };

  int64_t time;
  uint32_t id;
  EntityType type;
  float32_t x;
  float32_t y;
  int32_t data[4];
};

struct KeyboardEvent {
  enum KeyType {
    KEY_UP,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT
  };
  enum EventType {
    EVENT_KEYDOWN,
    EVENT_KEYUP
  };

  int64_t time;
  KeyType key_type;
  EventType event_type;
};

struct MouseEvent {
  enum ButtonType {
    BUTTON_LEFT,
    BUTTON_RIGHT
  };
  enum EventType {
    EVENT_KEYDOWN,
    EVENT_KEYUP
  };

  int64_t time;
  ButtonType button_type;
  EventType event_type;
  float32_t x, y;
};

}  // namespace bm

#endif  // BASE_PROTOCOL_H_
