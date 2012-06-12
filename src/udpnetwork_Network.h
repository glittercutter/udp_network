#pragma once

#include "udpnetwork_Common.h"
#include "udpnetwork_Packet.h"

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <deque>
#include <functional>
#include <unordered_map>

namespace udp_network
{

class Connection;

class Network
{
friend class Connection;

public:
    typedef std::function<bool(Connection*, const std::string&)> ConnectionRequestCb;
    typedef std::function<void(Connection*)> DisconnectionCb;

    Network(
        const ConnectionRequestCb& connect,
        const DisconnectionCb& disconnect,
        unsigned long currentTime,
        unsigned short port = 0);

    void update(unsigned long currentTime);

    std::string getStatus();
    bool isUp();

    Connection* connect(const std::string& address, const std::string& port);
    void disconnect(Connection* connection);

protected:
    Connection* createConnection(const boost::asio::ip::udp::endpoint& endpoint);
    void destroyConnection(Connection*, const std::string& info = "");
    Connection* getConnection(const boost::asio::ip::udp::endpoint& endpoint);

    void handleConnection(Buffer*, const boost::asio::ip::udp::endpoint& endpoint);
    void acceptConnection(Connection*);
    void requestConnection(Connection*);
    void refuseConnection(const boost::asio::ip::udp::endpoint& endpoint, const std::string& info = "");

    Buffer* send(const boost::asio::ip::udp::endpoint& endpoint); // AddressedPacket

    // NOTE: 'releaseBuffer' must be called before the returned buffer is discarded to avoid memory leak
    Buffer* newBuffer();
    void releaseBuffer(Buffer*);

    boost::asio::io_service mIoService;
    boost::asio::ip::udp::endpoint mEndpoint;
    boost::asio::ip::udp::socket mSocket;
    std::vector<Buffer*> mBuffers;

    std::unordered_map<boost::asio::ip::udp::endpoint, Connection*> mConnections;
    std::vector<AddressedPacket> mAddressedPackets;

    ConnectionRequestCb mConnectionRequestCb;
    DisconnectionCb mDisconnectionCb;

    unsigned mResponseTimeout;
    unsigned mConnectionTimeout;
    unsigned mPingRetryDelay;
    unsigned mConnectionRequestRetryDelay;
    unsigned mCurrentTime;
};

} // udp_network
