#ifndef BLOWMORPH_ENET_WRAPPER_EVENT_HPP_
#define BLOWMORPH_ENET_WRAPPER_EVENT_HPP_

#include <string>
#include <vector>

#include <enet/enet.h>

#include "base/macros.hpp"
#include "base/pstdint.hpp"

namespace bm {

class Enet;
class ClientHost;
class ServerHost;
class Peer;

// 'Event' class represents an event that can be delivered by
// 'ClientHost::Service' and 'ServerHost::Service' methods.
class Event {
  friend class ClientHost;
  friend class ServerHost;
  friend class Enet;

public:
  // TODO: use 'TYPE_' prefix instead of 'EVENT_'.
  enum EventType {
    // No event occurred within the specified time limit.
    EVENT_NONE,

    // A connection request initiated by 'ClientHost::Connect()' has completed.
    // You can use 'GetPeer()', 'GetPeerIp()', 'GetPeerPort()' methods to get
    // information about connected peer.
    EVENT_CONNECT,

    // A peer has disconnected. This event is generated on a successful
    // completion of a disconnect initiated by 'Peer::Disconnect()'.
    // You can use 'GetPeer()', 'GetPeerIp()', 'GetPeerPort()' methods to get
    // information about disconnected peer.
    EVENT_DISCONNECT,  

    // A packet has been received from a peer.
    // You can use 'GetPeer()', 'GetPeerIp()', 'GetPeerPort()' methods to get
    // information about peer which sent the packet.
    // 'GetChannelId()' returns the channel number upon which the packet was
    // received. 'GetData()' returns the data from received packet.
    // This packet must be destroyed with 'DestroyPacket()' after use.
    EVENT_RECEIVE
  };

  // TODO: get rid of this method.
  void DestroyPacket();

  // Returns the type of the event.
  EventType GetType() const;

  // Returns the channel id on the peer that generated the event.
  uint8_t GetChannelId() const;

  // Returns the data associated with the event.
  void GetData(std::vector<char>* output) const;

  // TODO: Get rid of this method.
  // Returns 'Peer', which caused the event. Returned 'Peer' should be
  // deallocated manually using 'delete'.
  // WARNING: This method allocates new 'Peer' even if a 'Peer' associated
  // with a remote peer already exists.
  Peer* GetPeer();

  // Use this instead of 'GetPeer()' if you need to call only 'Peer's getters.
  // The description of these methods can be found in 'Peer' declaration.
  std::string GetPeerIp() const;
  uint16_t GetPeerPort() const;
  void* GetPeerData() const;

private:
  DISALLOW_COPY_AND_ASSIGN(Event);

  // Creates an uninitialized 'Event'. Don't use it yourself.
  // You can create an 'Event' by using 'Enet::CreateEvent'.
  Event();

  ENetEvent _event;
};

} // namespace bm

#endif // BLOWMORPH_ENET_WRAPPER_EVENT_HPP_
