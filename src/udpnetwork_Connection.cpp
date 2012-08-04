#include "udpnetwork_Connection.h"
#include "udpnetwork_Network.h"

#include <iostream>
#include <sstream>

using namespace udp_network;

Connection::Connection(Network* network, const boost::asio::ip::udp::endpoint& endpoint, unsigned currentTime)
:   mNetwork(network), mEndpoint(endpoint),
    mPing(0), mReliableID(0), mUnreliableID(0),
    mReceivedReliableID(0), mReceivedUnreliableID(0),
    mSentTime(currentTime), mPingSentTime(currentTime),
    mHeartbeat(currentTime), mIsConnected(false), mUserData(0)
{}


Connection::~Connection()
{
    clear();
}


Buffer* Connection::send(bool reliable/* = false*/)
{
    Buffer* b = 0;

    if (reliable)
    {
        mReliablePackets.emplace_back();
        b = &mReliablePackets.back().buffer;
        b->setId(++mReliableID);
    }
    else
    {
        mUnreliablePackets.emplace_back();
        b = &mUnreliablePackets.back().buffer;
        b->setId(++mUnreliableID);
    }

    b->setType(PT_DATA); // Default type
    b->setReliable(reliable);

    return b;
}

std::vector<UnreliablePacket>& Connection::getUnreliable(unsigned currentTime)
{
    if (!mAcks.empty())
    {
        if (mUnreliablePackets.empty() && mReliablePackets.empty())
            send();

        for (unsigned ack : mAcks)
        {
            mUnreliablePackets.front().buffer.addAck(ack);
        }
        mAcks.clear();
    }

    if (!mUnreliablePackets.empty()) mSentTime = currentTime;
    return mUnreliablePackets;
}

std::list<ReliablePacket>& Connection::getReliable(unsigned currentTime)
{
    if (!mAcks.empty())
    {
        if (mUnreliablePackets.empty() && mReliablePackets.empty())
            send(true);

        for (unsigned ack : mAcks)
        {
            mReliablePackets.front().buffer.addAck(ack);
        }
        mAcks.clear();
    }

    if (!mReliablePackets.empty()) mSentTime = currentTime;
    return mReliablePackets;
}

void Connection::addReceivedBuffer(Buffer* b, unsigned currentTime)
{
    mHeartbeat = currentTime;

    if (b->getReliable())
    {
        while (b)
        {
            PacketId id = b->getId();

            if (id <= mReceivedReliableID)
            {
                // This packet is late (duplicated)
                return;
            }
            if (id > mReceivedReliableID + 1)
            {
                // This packet is early
                mUnorderedBufferCache.insert({id, b});
                return;
            }

            mAcks.push_back(id);
            ++mReceivedReliableID;
            mReceivedBuffers.push_back(b);
            
            auto ubcit = mUnorderedBufferCache.find(mReceivedReliableID);
            b = ubcit != mUnorderedBufferCache.end() ? ubcit->second : nullptr;
        }
    }
    else
    {
        mReceivedBuffers.push_back(b);
    }
}

void Connection::ack(PacketId id)
{
    auto it = std::find_if(
        mReliablePackets.begin(),
        mReliablePackets.end(),
        [&](const ReliablePacket& b) -> bool
        {
            return b.buffer.getId() == id;
        });

    if (it != mReliablePackets.end())
    {
        mReliablePackets.erase(it);
    }
}

void Connection::sendPing(unsigned currentTime)
{
    mPingSentTime = currentTime;
    send()->setType(PT_PING);
}

void Connection::handlePing()
{
    send()->setType(PT_PONG);
}

void Connection::handlePong(unsigned currentTime)
{
    mHeartbeat = currentTime;
}

void Connection::clear()
{
    for (auto b : mReceivedBuffers) mNetwork->releaseBuffer(b);
    mReceivedBuffers.clear();
    mUnreliablePackets.clear();
}


void Connection::disconnect()
{
    mNetwork->disconnect(this);
}


std::string Connection::printInfo()
{
    std::stringstream ss;
    ss << mEndpoint;
    return ss.str();
}
