// Copyright (c) 2013 Blowmorph Team

#ifndef SRC_BASE_PROTOCOL_H_
#define SRC_BASE_PROTOCOL_H_

#include "base/macros.h"
#include "base/pstdint.h"

namespace bm {

/*class TimeType {
public:
  TimeType() { }
  explicit TimeType(int32_t value) : value(value) { }
  
  TimeType operator+(const TimeType& dt) const {
    return TimeType(value + dt.value);
  }
  TimeType operator-(const TimeType& dt) const {
    return TimeType(value - dt.value);
  }
  
  int operator/(const TimeType& dt) const {
    return (int)(value / dt.value);
  }
  
  TimeType operator/(int dt) const {
    return TimeType(value / dt);
  }
  
  operator int32_t() const {
    return value;
  }
  
  bool operator >=(const TimeType& t) const {
    return value >= t.value;
  }
private:
  int32_t value;
};
SCHECK(sizeof(TimeType) == sizeof(int32_t));*/

typedef int64_t TimeType;

class Packet {
 public:
  enum Type {
    TYPE_UNKNOWN = 0,

    // S -> C. Followed by 'ClientOptions'.
    TYPE_CLIENT_OPTIONS,

    // S -> C. Followed by 'EntitySnapshot' with the entity description.
    TYPE_ENTITY_APPEARED,
    TYPE_ENTITY_UPDATED,
    TYPE_ENTITY_DISAPPEARED,

    // C -> S. Followed by 'KeyboardEvent'.
    TYPE_KEYBOARD_EVENT,
    // C -> S. Followed by 'MouseEvent'.
    TYPE_MOUSE_EVENT,

    // C -> S. Followed by 'TimeSyncData' filled with client time.
    TYPE_SYNC_TIME_REQUEST,
    // S -> C. Followed by 'TimeSyncData' filled with client and server time.
    TYPE_SYNC_TIME_RESPONSE,

    TYPE_MAX_VALUE
  };

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(Packet);
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
  TimeType client_time;
  TimeType server_time;
};

// type == EntitySnapshot::ENTITY_TYPE_PLAYER:
//   data[0] - health
//   data[1] - blow charge
//   data[2] - morph charge
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

  TimeType time;
  uint32_t id;
  EntityType type;
  float32_t x;
  float32_t y;
  int32_t data[3];
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

  TimeType time;
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

  TimeType time;
  ButtonType button_type;
  EventType event_type;
  float32_t x, y;
};

}  // namespace bm

#endif  // SRC_BASE_PROTOCOL_H_
