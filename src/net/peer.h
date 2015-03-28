// Copyright (c) 2015 Andrey Konovalov

#ifndef NET_PEER_H_
#define NET_PEER_H_

#include <string>

#include "base/macros.h"
#include "base/pstdint.h"

#include "net/dll.h"

struct _ENetPeer;

namespace bm {

class Enet;
class ClientHost;

// 'Peer' represents a remote transmission point which data packets
// may be sent or received from.
class Peer {
  friend class Event;
  friend class Host;

 public:
  // Queues a packet to be sent. 'data' is the allocated data for the packet.
  // 'length' is the length of the data. 'reliable' is the reliability flag.
  // 'channel_id' is the id of the channel the packet will be sent through.
  // The packet will be sent after calling 'ServerHost::Service()' or
  // 'ClientHost::Service()' methods.
  // Returns 'true' on success, returns 'false' on error.
  BM_NET_DECL bool Send(
    const char* data,
    size_t length,
    bool reliable = true,
    uint8_t channel_id = 0);

  // Returns the ip of the remote peer.
  // An empty string will be returned in case of an error.
  BM_NET_DECL std::string GetIp() const;

  // Returns the port of the remote peer.
  BM_NET_DECL uint16_t GetPort() const;

  // Request a disconnection from a peer.
  // An 'Event::TYPE_DISCONNECT' event will be generated by
  // 'ServerHost::Service()' or 'ClientHost::Service()' once
  // the disconnection is complete.
  BM_NET_DECL void Disconnect();

  // Force an immediate disconnection from a peer.
  // No 'Event::TYPE_DISCONNECT' event will be generated. The foreign peer
  // is not guarenteed to receive the disconnect notification, and is reset
  // immediately upon return from this function.
  BM_NET_DECL void DisconnectNow();

  // Request a disconnection from a peer, but only after all queued outgoing
  // packets are sent.
  // An 'Event::TYPE_DISCONNECT' event will be generated by
  // 'ServerHost::Service()' or 'ClientHost::Service()' once
  // the disconnection is complete.
  BM_NET_DECL void DisconnectLater();

  // Forcefully disconnects a peer. The foreign host represented by the peer
  // is not notified of the disconnection and will timeout on its connection
  // to the local host.
  BM_NET_DECL void Reset();

  // Sets the 'Peer''s internal data, that can be freely modified.
  // If you have two intances of 'Peer' associated with the same '_peer', both
  // of them will have the same internal data.
  BM_NET_DECL void SetData(void* data);

  // Returns the 'Peer's internal data.
  BM_NET_DECL void* GetData() const;

 private:
  // Creates a 'Peer' associated with the ENet peer 'peer'.
  explicit Peer(_ENetPeer* peer);

  _ENetPeer* _peer;

  DISALLOW_COPY_AND_ASSIGN(Peer);
};

}  // namespace bm

#endif  // NET_PEER_H_