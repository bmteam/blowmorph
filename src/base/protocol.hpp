#ifndef BLOWMORPH_BASE_PROTOCOL_HPP_
#define BLOWMORPH_BASE_PROTOCOL_HPP_

#include "base/pstdint.hpp"

namespace bm {

// TODO: think about this namespace.
namespace protocol {

enum PacketType {
  BM_PACKET_UNKNOWN = 0,

  // S -> C. Followed by 'ClientOptions'.
  BM_PACKET_CLIENT_OPTIONS,

  // S -> C. Followed by 'EntitySnapshot' with the entity description.
  BM_PACKET_ENTITY_APPEARED,
  BM_PACKET_ENTITY_UPDATED,
  BM_PACKET_ENTITY_DISAPPEARED,

  // C -> S. Followed by 'KeyboardEvent'.
  BM_PACKET_KEYBOARD_EVENT,
  // C -> S. Followed by 'MouseEvent'.
  BM_PACKET_MOUSE_EVENT,

  // C -> S. Followed by 'TimeSyncData' filled with client time.
  BM_PACKET_SYNC_TIME_REQUEST,
  // S -> C. Followed by 'TimeSyncData' filled with client and server time.
  BM_PACKET_SYNC_TIME_RESPONSE,

  BM_PACKET_MAX_VALUE
};

enum EntityType {
  BM_ENTITY_UNKNOWN = 0,

  BM_ENTITY_PLAYER,
  BM_ENTITY_BULLET,
  BM_ENTITY_WALL,
  BM_ENTITY_DUMMY,

  BM_ENTITY_MAX_VALUE
};

struct ClientOptions {
  uint32_t id;
  float speed;
  float x, y;
};

struct TimeSyncData {
  uint32_t client_time;
  uint32_t server_time;
};

struct EntitySnapshot {
  uint32_t time;
  uint32_t id;
  EntityType type;
  float x;
  float y;
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

  uint32_t time;
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

  uint32_t time;
  ButtonType button_type;
  EventType event_type;
  float x, y;
};

} // namespace protocol

} //namespace bm

#endif //BLOWMORPH_BASE_PROTOCOL_HPP_