#ifndef BLOWMORPH_ENET_WRAPPER_CLIENT_HOST_HPP_
#define BLOWMORPH_ENET_WRAPPER_CLIENT_HOST_HPP_

#include <string>

#include <enet/enet.h>

#include "base/macros.hpp"
#include "base/pstdint.hpp"

#include "library.hpp"

namespace bm {

class Enet;
class Event;
class Peer;

// A client host for communicating with a server host.
class ClientHost {
  friend class Enet;

public:
  BM_ENET_DECL ~ClientHost();

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

  // Initiates connection procedure to another host. To complete connection,
  // an event 'EVENT_CONNECT' should be dispatched using 'Service()'.
  // 'channel_count' is the number of channels to be used.
  // Returns 'Peer' on success, returns 'NULL' on error.
  // Returned 'Peer' should be deallocated manually using 'delete'.
  BM_ENET_DECL Peer* Connect(std::string server_ip, uint16_t port, size_t channel_count = 1);

  // This function is needed only be used in circumstances where one wishes to
  // send queued packets earlier than in a call to 'ServerHost::Service()'.
  BM_ENET_DECL void Flush();

  // Cleans up. Automatically called in the destructor.
  BM_ENET_DECL void Destroy();

private:
  DISALLOW_COPY_AND_ASSIGN(ClientHost);

  // Creates an uninitialized 'ClientHost'. This is an internal constructor
  // used by 'ClientHost::Create'. Don't use it yourself.
  // You can create a 'ClientHost' by using 'Enet::CreateClientHost'.
  ClientHost();

  // Creates 'ClientHost'.
  // You may specify 'channel_count' - number of channels to be used.
  // You may specify incoming and outgoing bandwidth of the server in bytes
  // per second. Specifying '0' for these two options will cause ENet to rely
  // entirely upon its dynamic throttling algorithm to manage bandwidth.
  // Returned 'ClientHost' should be deallocated manually using 'delete'.
  static ClientHost* Create(
    size_t channel_count = 1,
    uint32_t incoming_bandwidth = 0,
    uint32_t outgoing_bandwidth = 0
  );

  enum {
    STATE_CREATED, 
    STATE_INITIALIZED,
    STATE_DESTROYED
  } _state;

  ENetHost* _client;
};

} // namespace bm

#endif // BLOWMORPH_ENET_WRAPPER_CLIENT_HOST_HPP_