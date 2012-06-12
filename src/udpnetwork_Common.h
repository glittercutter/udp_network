#pragma once

#include <boost/asio/ip/udp.hpp>
#include <boost/functional/hash.hpp>

namespace udp_network
{

typedef unsigned char byte;

inline std::size_t hash_endpoint(const boost::asio::ip::udp::endpoint& e)
{
    std::size_t h = 0;
    boost::hash_combine(h, e.address().to_v4().to_ulong());
    boost::hash_combine(h, e.port());
    return h;
}

} // udp_network

namespace std
{

template<>
struct hash<boost::asio::ip::udp::endpoint> : public unary_function<boost::asio::ip::udp::endpoint, size_t>
{
    std::size_t operator() (const boost::asio::ip::udp::endpoint& e) const { return udp_network::hash_endpoint(e); }
};

template<>
struct equal_to<boost::asio::ip::udp::endpoint> : public unary_function<boost::asio::ip::udp::endpoint,size_t>
{
    bool operator() (const boost::asio::ip::udp::endpoint& e1, const boost::asio::ip::udp::endpoint& e2) const
    {
        return udp_network::hash_endpoint(e1) == udp_network::hash_endpoint(e2);
    }
};

} // std
