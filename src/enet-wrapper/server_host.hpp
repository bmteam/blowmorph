#ifndef BLOWMORPH_ENET_WRAPPER_SERVER_HOST_HPP_
#define BLOWMORPH_ENET_WRAPPER_SERVER_HOST_HPP_

#include <enet/enet.h>

#include "base/macros.hpp"
#include "base/pstdint.hpp"

#include "library.hpp"

namespace bm {

class Enet;
class Event;

// A server host for communicating with client hosts.
class ServerHost {
  friend class Enet;

public:
  BM_ENET_DECL ~ServerHost();

  // Broadcast data from 'data' with length of 'length' to all Peer's on 
  // selected channel, associated with 'channel_id'.
  // Returns 'true' on success, returns 'false' on error.
  BM_ENET_DECL bool Broadcast(const char* data, size_t length, bool reliable = true, uint8_t channel_id = 0);

  // Checks for events with a timeout. Should be called to send all queued
  // with 'Peer::Send()' packets. 'event' is an 'Event' class where event
  // details will be placed if one occurs.
  // If a timeout of '0' is specified, 'Service()' will return immediately
  // if there are no events to dispatch. If 'event' is 'NULL' then no events
  // will be delivered.
  // An 'Event' with type 'EVENT_NONE' will be placed in 'event' if no event
  // occured within the specified time limit.
  // Returns 'true' on success, returns 'false' on error.
  BM_ENET_DECL bool Service(Event* event, uint32_t timeout);

  // This function need only be used in circumstances where one wishes to send
  // queued packets earlier than in a call to ClientHost::Service().
  BM_ENET_DECL void Flush();

  // Cleans up. Automatically called in the destructor.
  BM_ENET_DECL void Destroy();

private:
  DISALLOW_COPY_AND_ASSIGN(ServerHost);

  // Creates an uninitialized 'ServerHost'. This is an internal constructor
  // used by 'ServerHost::Create'. Don't use it yourself.
  // You can create a 'ServerHost' by using 'Enet::CreateServerHost'.
  ServerHost();

  // Creates 'ServerHost' on port 'port'.
  // You may specify 'channel_count' - number of channels to be used.
  // You may specify incoming and outgoing bandwidth of the server in bytes
  // per second. Specifying '0' for these two options will cause ENet to rely
  // entirely upon its dynamic throttling algorithm to manage bandwidth.
  // Returned 'ServerHost' should be deallocated manually.
  static ServerHost* Create(
    uint16_t port,
    size_t peer_count = 32,
    size_t channel_count = 1,
    uint32_t incoming_bandwidth = 0,
    uint32_t outgoing_bandwidth = 0
  );

  enum {
    STATE_CREATED, 
    STATE_INITIALIZED,
    STATE_DESTROYED
  } _state;

  ENetHost* _server;
};

} // namespace bm

#endif //BLOWMORPH_ENET_WRAPPER_SERVER_HOST_HPP_