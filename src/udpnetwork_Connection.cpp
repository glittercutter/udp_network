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
        if (!mReliablePackets.empty() &&
            !mReliablePackets.back().wasSent)
        {
            // Do not create another packet
            return &mReliablePackets.back().buffer;
        }

        mReliablePackets.emplace_back();
        b = &mReliablePackets.back().buffer;
        b->setId(++mReliableID);
    }
    else
    {
        if (!mUnreliablePackets.empty())
        {
            // Do not create another packet
            return &mReliablePackets.back().buffer;
        }

        mUnreliablePackets.emplace_back();
        b = &mUnreliablePackets.back().buffer;
        b->setId(++mUnreliableID);
    }

    b->setType(PT_DATA); // Default type
    b->setReliable(reliable);

    return b;
}

void Connection::send(unsigned long time, boost::asio::ip::udp::socket& socket)
{
    // Write ack
    if (!mAcks.empty())
    {
        if (!mUnreliablePackets.empty()) 
        {
            for (auto id : mAcks) mUnreliablePackets.back().buffer.addAck(id);
        }
        else if (!mReliablePackets.empty())
        {
            for (auto id : mAcks) mReliablePackets.back().buffer.addAck(id);
        }
        else
        {
            send(false); // Create new unreliable packet if no packet are queued for sending.
            std::cout<<"mUnreliablePackets.size()="<<mUnreliablePackets.size()<<std::endl;
            for (auto id : mAcks) mUnreliablePackets.back().buffer.addAck(id);
        }

        mAcks.clear();
    }

    if (!mReliablePackets.empty() || !mUnreliablePackets.empty()) mSentTime = time;

    //
    // Send
    //

    boost::system::error_code errorCode;
    
    // Unreliable
    {
        for (auto& p : mUnreliablePackets)
        {
            std::cout<<"Sending unreliable packet"<<std::endl;
            p.buffer.finalize();
            socket.send_to(
                boost::asio::buffer(p.buffer.data(), p.buffer.size()),
                mEndpoint, 0, errorCode);
        }
    }

    // Reliable
    {
        for (auto& p : mReliablePackets)
        {
            // Send reliable packet.
            // Resend packet on timeout.
            if (!p.wasSent || time - p.time >= mPing)
            {
                std::cout<<"Sending reliable packet"<<std::endl;
                p.buffer.finalize();
                socket.send_to(
                    boost::asio::buffer(p.buffer.data(), p.buffer.size()),
                    mEndpoint, 0, errorCode);

                p.time = time;
                p.wasSent = true;
            }
        }
    }

    clear();
}

void Connection::addIncomingBuffer(Buffer* b, unsigned currentTime)
{
    mHeartbeat = currentTime;

    if (b->getReliable())
    {
        while (b)
        {
            PacketId id = b->getId();
            std::cout<<"Received Packet id: "<<(short)id<<std::endl;

            if (id <= mReceivedReliableID)
            {
                // This packet is late (duplicated)
                return;
            }
            if (id > mReceivedReliableID + 1)
            {
                // This packet is early
                mUnorderedBufferCache.insert({id, b});
                std::cout<<"Early packet received: num cached:"<<mUnorderedBufferCache.size()<<std::endl;
                return;
            }

            mAcks.push_back(id);
            ++mReceivedReliableID;
            mReceivedBuffers.push_back(b);
            std::cout<<"Received reliable packet: id:"<<(unsigned)id<<std::endl;
            
            auto ubcit = mUnorderedBufferCache.find(mReceivedReliableID);
            if (ubcit != mUnorderedBufferCache.end())
            {
                b = ubcit->second;
                mUnorderedBufferCache.erase(ubcit);
            }
            else
            {
                b = nullptr;
            }
        }
    }
    else
    {
        mReceivedBuffers.push_back(b);
    }
}

void Connection::ack(PacketId id)
{
    std::cout<<__PRETTY_FUNCTION__<<" id:"<<(unsigned)id<<std::endl;

    auto it = std::find_if(
        mReliablePackets.begin(),
        mReliablePackets.end(),
        [&](const ReliablePacket& p) { return p.buffer.getId() == id; });

    if (it != mReliablePackets.end())
    {
        std::cout<<"Reliable packet acked: "<<(int)id<<std::endl;
        mReliablePackets.erase(it);
    }
}

void Connection::sendPing(unsigned currentTime)
{
    mPingSentTime = currentTime;
    send()->setType(PT_PING);
    std::cout<<__PRETTY_FUNCTION__<<std::endl;
}

void Connection::handlePing()
{
    send()->setType(PT_PONG);
    std::cout<<__PRETTY_FUNCTION__<<std::endl;
}

void Connection::handlePong(unsigned currentTime)
{
    mHeartbeat = currentTime;
    std::cout<<__PRETTY_FUNCTION__<<std::endl;
}

void Connection::clear()
{
    for (auto b : mReceivedBuffers) mNetwork->releaseBuffer(b);
    mReceivedBuffers.clear();
    mUnreliablePackets.clear();
    std::cout<<__PRETTY_FUNCTION__<<std::endl;
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
