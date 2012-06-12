#pragma once

#include "udpnetwork_Common.h"

#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/array.hpp>
#include <deque>
#include <string>
#include <stdint.h>

namespace udp_network
{

// The type and flag(s) is stored on the first byte of each packet

enum PacketTypes
{
    PT_PING = 0,
    PT_PONG,
    PT_CONNECTION,
    PT_DATA,
};

enum PacketFlag
{
    PF_RELIABLE     = 8,
    PF_HAS_ACK      = 16,
    //PF_PLACEHOLDER = 32;
    //PF_PLACEHOLDER = 64;
    //PF_PLACEHOLDER = 128;
};

enum ConnectionMessage
{
    CM_REQUEST,
    CM_ACCEPT,
    CM_REFUSE,
    CM_DISCONNECT
};

typedef byte PacketType;
typedef uint16_t PacketId;
typedef int32_t Number_t;

const unsigned PacketTypePosition = 0;
const unsigned PacketIdPosition = PacketTypePosition + sizeof(PacketType);
const unsigned PacketHeaderSize = PacketIdPosition + sizeof(PacketId);
const unsigned PacketFlagCount = 5;

class Buffer
{
public:
    static const std::size_t Size = 1024;
    typedef boost::array<byte, Size> Data;
    static const unsigned short InvalidBoolByteIt = -1;

    //
    // Write
    //
    Buffer& operator << (const bool val) { writeBool(&val); return *this; }

    Buffer& operator << (const int8_t val) { write8(&val); return *this; }
    Buffer& operator << (const uint8_t val) { write8(&val); return *this; }

    Buffer& operator << (const int16_t val) { write16(&val); return *this; }
    Buffer& operator << (const uint16_t val) { write16(&val); return *this; }

    Buffer& operator << (const int32_t val) { write32(&val); return *this; }
    Buffer& operator << (const uint32_t val) { write32(&val); return *this; }

    Buffer& operator << (const float val) { writeFloat(&val); return *this; }
    Buffer& operator << (const std::string& val) { writeString(val); return *this; }

    void writeBool(const bool val) { writeBool(&val); }
    void writeByte(const byte val) { write8(&val); }
    void writeShort(const short val) { write16(&val); }
    void writeInt(const int val) { write32(&val); }
    void writeFloat(const float val) { writeFloat(&val); }

    // Containers
    template <class T>
    Buffer& operator << (const std::vector<T> v)
    {
        // TODO this limit to a size of 256 elements
        *this << (unsigned char)v.size();
        for (size_t i = 0; i < v.size(); i++)
            *this << v[i];
        return *this;
    }

    //
    // Read
    //
    Buffer& operator >> (bool& val) { readBool(&val); return *this; }

    Buffer& operator >> (int8_t& val) { read8(&val); return *this; }
    Buffer& operator >> (uint8_t& val) { read8(&val); return *this; }

    Buffer& operator >> (int16_t& val) { read16(&val); return *this; }
    Buffer& operator >> (uint16_t& val) { read16(&val); return *this; }

    Buffer& operator >> (int32_t& val) { read32(&val); return *this; }
    Buffer& operator >> (uint32_t& val) { read32(&val); return *this; }

    Buffer& operator >> (float& val) { readFloat(&val); return *this; }
    Buffer& operator >> (std::string& val) { readString(val); return *this; }

    void readBool(bool& val) { readBool(&val); }
    void readByte(byte& val) { read8(&val); }
    void readShort(short& val) { read16(&val); }
    void readInt(int& val) { read32(&val); }
    void readFloat(float& val) { readFloat(&val); }

    // Alternative returning value
    bool readBool() { bool val; readBool(&val); return val; }
    byte readByte() { byte val; read8(&val); return val; }
    short readShort() { short val; read16(&val); return val; }
    int readInt() { int val; read32(&val); return val; }
    float readFloat() { float val; readFloat(&val); return val; }

    // Containers
    template <class T>
    Buffer& operator >> (std::vector<T> v)
    {
        unsigned char size;
        *this>>size;
        v.resize(size);
        for (size_t i = 0; i < size; i++)
        {
            T t;
            *this >> t;
            v[i] = t;
        }
        return *this;
    }
    
    // ********************************************************************
    // The functions below shoudn't need to be used from ouside the library
    // ********************************************************************

    void writeBool(const bool* v);
    void write8(const void* v);
    void write16(const void* v);
    void write32(const void* v);
    void writeFloat(const float* v);
    void writeString(const std::string& v);

    void readBool(bool* v);
    void read8(void* v);
    void read16(void* v);
    void read32(void* v);
    void readFloat(float* v);
    void readString(std::string& v);

    Buffer() { clear(); }

    Data& data() { return mData; }
    std::size_t size() { return mSize; }

    void data(const Data& d) { mData = d; }
    void size(std::size_t s) { mSize = s; }
    
    std::size_t getHeaderSize();
    void addAck(PacketId);
    bool hasAck();
    byte getAckCount();
    PacketId getAck(byte);
    bool getReliable() const;
    void setReliable(bool);
    byte getType() const;
    void setType(byte);
    PacketId getId() const;
    void setId(PacketId);

    std::string debugHeader();
    void clear();
    void finalize();

protected:
    void incrementBool(bool write = false);

    Data mData;
    unsigned short mSize;
    unsigned short mByteIt;
    unsigned short mBoolByteIt;
    unsigned short mBoolBitIt;
    unsigned char mNumberOfAck; 
};

struct Packet
{
    Buffer buffer;
};

struct UnreliablePacket : public Packet
{
};

struct ReliablePacket : public Packet
{
    unsigned time;
};

struct AddressedPacket : public UnreliablePacket
{
    AddressedPacket(const boost::asio::ip::udp::endpoint& endpoint)
    :   endpoint(endpoint) {}

    boost::asio::ip::udp::endpoint endpoint;
};

} // udp_network
