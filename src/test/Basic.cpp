#include "../Connection.h"
#include "../Network.h"
#include "../utils/ReplicatedVariable.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <functional>
#include <thread>
#include <chrono>
#include <set>

using std::cout;
using std::endl;


enum class MessageHeader : unsigned char
{
    Primitive,
    Replicated
};

class ReplicatedData
{
public:
    ReplicatedData()
    :   mReplicatedVariables(),
        mInt(mReplicatedVariables.add<int>(20)),
        mInt2(mReplicatedVariables.add<int>(24098))
    {}
    
    friend udp_network::Buffer& operator >> (udp_network::Buffer& b, ReplicatedData& d)
    {
        b>>d.mReplicatedVariables;
        return b;
    }
    friend udp_network::Buffer& operator << (udp_network::Buffer& b, ReplicatedData& d)
    {
        b<<d.mReplicatedVariables;
        return b;
    }

    udp_network::ReplicatedVariableContainer mReplicatedVariables;
    udp_network::ReplicatedVariable<int>& mInt;
    udp_network::ReplicatedVariable<int>& mInt2;
};

unsigned g_loop_rate_millisecond = 250;
std::set<udp_network::Connection*> g_connections;
ReplicatedData g_replicated_data;


bool onConnectionRequest(udp_network::Connection* c, const std::string& info)
{
    cout<<"accepting request"<<endl;
    g_connections.insert(c);
    return true;
}


void onDisconnection(udp_network::Connection* c)
{
    g_connections.erase(c);
    cout<<"disconnected"<<endl;
}


void printHelp()
{
    cout<<"Option:"<<endl;
    cout<<"   --port <port number>"<<endl;
    cout<<"   --host (server address)"<<endl;
    cout<<"   --server (run as server)"<<endl;
    throw "Exiting";
}


void sendMessage()
{
    static int count = 0;
    static const int primitive_count = 5;
    static const int reliable_count = 15;

    if (count < primitive_count)
    {
        auto& b = *(*g_connections.begin())->send(true); // Create new reliable packet
        b<<(unsigned char)MessageHeader::Primitive;
        b<<true<<false<<std::string("testing, testing")<<2457544<<2334.53344f;
    }
    else if (count < reliable_count)
    {
        if (count == reliable_count-4) g_replicated_data.mInt.set(10);
        if (count == reliable_count-3) g_replicated_data.mInt.set(10); // Same value, is not sent
        if (count == reliable_count-2) g_replicated_data.mInt.set(11);

        auto& b = *(*g_connections.begin())->send(true); // Create new reliable packet
        b<<(unsigned char)MessageHeader::Replicated;
        b<<g_replicated_data;
    }
    else
    {
        (*g_connections.begin())->disconnect();
    }

    ++count;
}


void receiveMessage()
{
    for (auto cit : g_connections)
    {
        auto& buffs = cit->getReceivedBuffers();
        for (auto bptr : buffs)
        {
            auto& b = *bptr;

            unsigned char packetHeader;
            b>>packetHeader; // Read packet type

            switch ((MessageHeader)packetHeader)
            {
                case MessageHeader::Primitive:
                {
                    bool bool1;
                    bool bool2;
                    std::string string1;
                    int int1;
                    float float1;
                    
                    b>>bool1>>bool2>>string1>>int1>>float1;

                    cout<<bool1<<endl;
                    cout<<bool2<<endl;
                    cout<<string1<<endl;
                    cout<<int1<<endl;
                    cout<<float1<<endl;

                    break;
                }
                case MessageHeader::Replicated:
                {
                    cout<<"received replicated"<<endl;
                    b>>g_replicated_data;
                    break;
                }
            }
        }
    }
}


int main(int argc, char** argv)
{
    bool server = false;
    std::string port("0");
    std::string host("0");

    if (argc <= 1) printHelp();

    // Parse arguments
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--server")) server = true;
        else if (!strcmp(argv[i], "--port") && ++i < argc) port = argv[i];
        else if (!strcmp(argv[i], "--host") && ++i < argc) host = argv[i];
        else printHelp();
    }

    if (server) cout<<"Running as server"<<endl;
    else cout<<"Running as client"<<endl;

    // Setup time
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime(std::chrono::high_resolution_clock::now()); 
    std::chrono::milliseconds milliseconds; 

    // Start the network
    udp_network::Network network(
        std::bind(&onConnectionRequest, std::placeholders::_1, std::placeholders::_2),
        std::bind(&onDisconnection, std::placeholders::_1),
        milliseconds.count(), server ? atoi(port.c_str()) : 0);

    cout<<network.getStatus()<<endl;

    // Connect to the server if we are a client
    if (!server)
    {
        g_connections.insert(network.connect(host, port));
    }

    bool clientIsConnectedToServer = false;

    while (42)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(g_loop_rate_millisecond));
        milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime);
        network.update(milliseconds.count());

        if (!server && !g_connections.empty())
        {
            if (!clientIsConnectedToServer)
            {
                cout << "Connection status = ";
                clientIsConnectedToServer = (*g_connections.begin())->isConnected();
                if (!clientIsConnectedToServer)
                {
                    cout<<"Not connected"<<endl;
                    continue;
                }
                cout<<"Connected"<<endl;
            }
            sendMessage();
        }
        else receiveMessage();
    }

    return 0;
}
