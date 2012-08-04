#include "udpnetwork_Network.h"
#include "udpnetwork_Connection.h"

using namespace udp_network;

Network::Network(
    const ConnectionRequestCb& connect,
    const DisconnectionCb& disconnect,
    unsigned long currentTime,
    unsigned short port/* = 0*/)

:   mIoService(),
    mEndpoint(boost::asio::ip::udp::v4(), port),
    mSocket(mIoService, mEndpoint),
    mConnectionRequestCb(connect),
    mDisconnectionCb(disconnect),
    mResponseTimeout(2000),
    mConnectionTimeout(5000),
    mPingRetryDelay(1000),
    mConnectionRequestRetryDelay(1000),
    mCurrentTime(currentTime),
    bUpdateInProgress(false)
{
    mSocket.non_blocking(true);
}

Connection* Network::connect(const std::string& addr, const std::string& port)
{
    boost::asio::ip::udp::resolver resolver(mIoService);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), addr, port);
    auto c = createConnection(*resolver.resolve(query));
    requestConnection(c);
    return c;
}

void Network::disconnect(Connection* c)
{
    destroyConnection(c);
}

void Network::update(unsigned long currentTime)
{
    bUpdateInProgress = true;
    mCurrentTime = currentTime;
    boost::system::error_code errorCode;
    boost::asio::ip::udp::endpoint endpoint;

    ////////////////////////
    // Send packet for all connection
    ////////////////////////
    for (auto cit = mConnections.begin(); cit != mConnections.end(); )
    {
        Connection* c = cit->second;

        if (!c->isConnected() &&
            c->getSentTime() + mConnectionRequestRetryDelay <= currentTime)
        {
            requestConnection(c);
        }
        else if (c->getHeartbeat() + mConnectionTimeout <= currentTime)
        {
            destroyConnection(c, "connection timeout");
            ++cit;
            c->clear();
            continue;
        }
        else if (c->getHeartbeat() + mResponseTimeout <= currentTime &&
                 c->getPingSentTime() + mPingRetryDelay <= currentTime)
        {
            c->sendPing(currentTime);
        }

        // Unreliable
        {
            auto& packets = c->getUnreliable(currentTime);
            for (UnreliablePacket& p : packets)
            {
                p.buffer.finalize();
                mSocket.send_to(
                    boost::asio::buffer(p.buffer.data(), p.buffer.size()),
                    c->getEndpoint(), 0, errorCode);
            }
        }

        // Reliable
        {
            auto& packets = c->getReliable(currentTime);
            for (ReliablePacket& p : packets)
            {
                if (currentTime - p.time > c->getPing())
                {
                    p.buffer.finalize();
                    mSocket.send_to(
                        boost::asio::buffer(p.buffer.data(), p.buffer.size()),
                        c->getEndpoint(), 0, errorCode);

                    p.time = currentTime;
                }
            }
        }
        
        c->clear();
        ++cit;
    }

    for (auto& b : mAddressedPackets)
    {
        mSocket.send_to(
            boost::asio::buffer(b.buffer.data(), b.buffer.size()),
            b.endpoint, 0, errorCode);
    }
    mAddressedPackets.clear();

    ////////////////////////
    // Receive
    ////////////////////////
    Buffer* buffer = newBuffer();
    while (42)
    {
        buffer->size(mSocket.receive_from(
            boost::asio::buffer(buffer->data(), Buffer::Size),
            endpoint, 0, errorCode));

        if (!buffer->size()) break; // Nothing was received

        Connection* connection = getConnection(endpoint);
        if (connection) 
        {
            if (buffer->hasAck())
            {
                for (unsigned char i = 0; i < buffer->getAckCount(); i++)
                    connection->ack(buffer->getAck(i));
            }
        }

        switch (buffer->getType())
        {
            case PT_PING:
                if (connection) connection->handlePing();
                break;

            case PT_PONG:
                if (connection) connection->handlePong(currentTime);
                break;

            case PT_CONNECTION:
                handleConnection(buffer, endpoint);
                break;

            case PT_DATA:
                if (connection)
                {
                    // The buffer ownership is transfered to the connection
                    // TODO ???? eliminate this and use a callback for the connection to parse the packet ???????
                    connection->addReceivedBuffer(buffer, currentTime);
                    buffer = newBuffer(); // Get a new one
                }
                break;

            default:
                break;
        }
    }
    releaseBuffer(buffer);

    bUpdateInProgress = false;
    runQueuedJobs();
}

void Network::handleConnection(Buffer* buffer, const boost::asio::ip::udp::endpoint& endpoint)
{
    std::string info;
    switch (buffer->readByte())
    {
        case CM_REQUEST:
        {
            auto c = createConnection(endpoint);

            if (!mConnectionRequestCb(c, info))
            {
                destroyConnection(c);
                refuseConnection(endpoint, info);
            }
            else acceptConnection(c);
            break;
        }
        case CM_ACCEPT:
            if (auto c = getConnection(endpoint))
                c->setConnected(true);
            break;

        case CM_REFUSE:
            if (auto c = getConnection(endpoint))
                destroyConnection(c);
            break;

        case CM_DISCONNECT:
            if (auto c = getConnection(endpoint))
                destroyConnection(c);
            break;
    }
}

Connection* Network::createConnection(const boost::asio::ip::udp::endpoint& endpoint)
{
    if (auto c = getConnection(endpoint))
    {
        return c; // Already connected
    }

    auto c = new Connection(this, endpoint, mCurrentTime);
    mConnections.insert({endpoint, c});
    return c;
}

void Network::destroyConnection(Connection* c, const std::string& info/* = ""*/)
{
    if (bUpdateInProgress)
    {
        mQueuedJobs.push_back(std::bind(&Network::destroyConnection,this,c,info));
        return;
    }

    mDisconnectionCb(c);

    auto b = send(c->getEndpoint());
    b->setType(PT_CONNECTION);
    b->writeByte(CM_DISCONNECT);
    b->writeString(info);

    mConnections.erase(c->getEndpoint());
    delete c;
}

void Network::acceptConnection(Connection* c)
{
    auto b = c->send();
    b->setType(PT_CONNECTION);
    b->writeByte(CM_ACCEPT);
    c->setConnected(true);
}

void Network::refuseConnection(const boost::asio::ip::udp::endpoint& endpoint, const std::string& info/* = ""*/)
{
    auto b = send(endpoint);
    b->setType(PT_CONNECTION);
    b->writeByte(CM_REFUSE);
    b->writeString(info);
}

void Network::requestConnection(Connection* c)
{
    auto b = c->send();
    b->setType(PT_CONNECTION);
    b->writeByte(CM_REQUEST);
}

Connection* Network::getConnection(const boost::asio::ip::udp::endpoint& endpoint)
{
    auto it = mConnections.find(endpoint);
    if (it == mConnections.end()) return 0;
    return it->second;
}

Buffer* Network::send(const boost::asio::ip::udp::endpoint& endpoint)
{
    mAddressedPackets.emplace_back(endpoint);
    return &mAddressedPackets.back().buffer;
}

std::string Network::getStatus()
{
    std::stringstream ss;
    if (mSocket.is_open())
    {
        ss << "Socket opened on address: " << mSocket.local_endpoint().address().to_string();
        ss << ", port: " << mSocket.local_endpoint().port();
    }
    else ss << "Socket is not opened";
    return ss.str();
}

bool Network::isUp()
{
    return mSocket.is_open();
}

Buffer* Network::newBuffer()
{
    //std::cout<<"num of buffers:"<<mBuffers.size()<<std::endl;
    Buffer* ret = nullptr;
    if (mBuffers.empty()) ret = new Buffer();
    else
    {
        ret = mBuffers.back();
        mBuffers.pop_back();
        ret->clear();
    }
    return ret;
}

void Network::releaseBuffer(Buffer* b)
{
    mBuffers.push_back(b);
}


void Network::runQueuedJobs()
{
    if (bUpdateInProgress) return;

    while (!mQueuedJobs.empty()) 
    {
        mQueuedJobs.front()();
        mQueuedJobs.pop_front();
    }
}
