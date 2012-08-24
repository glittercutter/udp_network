#pragma once

#include "udpnetwork_Packet.h"

#include <boost/asio/ip/udp.hpp>
#include <unordered_map>
#include <list>

namespace udp_network
{

class Network;

class Connection 
{
friend class Network;

public:
    Connection(Network* network, const boost::asio::ip::udp::endpoint& endpoint, unsigned currentTime);
    ~Connection();

    Buffer* send(bool reliable = false);
    std::vector<Buffer*>& getIncomingBuffers() { return mReceivedBuffers; }
    const boost::asio::ip::udp::endpoint& getEndpoint() { return mEndpoint; }

    unsigned getPing() { return mPing; }
    unsigned getHeartbeat() { return mHeartbeat; }
    unsigned getSentTime() { return mSentTime; }
    unsigned getPingSentTime() { return mPingSentTime; }
    
    void* getUserData() { return mUserData; }
    void setUserData(void* data) { mUserData = data; }

    bool isConnected() { return mIsConnected; }
    void disconnect();

    std::string printInfo();

protected:
    void addIncomingBuffer(Buffer* buff, unsigned currentTime);
    void send(unsigned long time, boost::asio::ip::udp::socket& socket);
    
    void sendPing(unsigned currentTime);
    void handlePing();
    void handlePong(unsigned currentTime);

    void ack(unsigned short id);
    void setConnected(bool state = true) { mIsConnected = state; }
    void clear();

private:
    Network* mNetwork;
    boost::asio::ip::udp::endpoint mEndpoint;

    std::vector<Buffer*> mReceivedBuffers;
    std::unordered_map<unsigned short, Buffer*> mUnorderedBufferCache;
    std::vector<UnreliablePacket> mUnreliablePackets;
    std::list<ReliablePacket> mReliablePackets;

    std::vector<PacketId> mAcks;

    unsigned mPing;
    unsigned short mReliableID;
    unsigned short mUnreliableID;
    unsigned short mReceivedReliableID;
    unsigned short mReceivedUnreliableID;

    unsigned mSentTime;
    unsigned mPingSentTime;
    unsigned mHeartbeat;
    bool mIsConnected;
    void* mUserData;
};

} // udp_network
