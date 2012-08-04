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

    struct BoolIterator
    {
        BoolIterator(unsigned bytePos, byte bitPos)
        :   BytePosition(bytePos), BitPosition(bitPos) {}
        unsigned BytePosition;
        byte BitPosition;
    };

    struct ByteIterator
    {
        ByteIterator(unsigned bytePos)
        :   BytePosition(bytePos) {}
        unsigned BytePosition;
    };

    bool eof() { return mByteIt >= mSize-1; }

    void eraseLastByte();
    void eraseLastShort();
    void eraseLastInt();
    void eraseLastFloat();

    // Write
    inline Buffer& operator << (const bool val) { writeBool(&val); return *this; }
    //
    inline Buffer& operator << (const int8_t val) { write8(&val); return *this; }
    inline Buffer& operator << (const uint8_t val) { write8(&val); return *this; }
    //
    inline Buffer& operator << (const int16_t val) { write16(&val); return *this; }
    inline Buffer& operator << (const uint16_t val) { write16(&val); return *this; }
    //
    inline Buffer& operator << (const int32_t val) { write32(&val); return *this; }
    inline Buffer& operator << (const uint32_t val) { write32(&val); return *this; }
    //
    inline Buffer& operator << (const float val) { writeFloat(&val); return *this; }
    inline Buffer& operator << (const std::string& val) { writeString(val); return *this; }

    inline void writeBool(const bool val) { writeBool(&val); }
    inline void writeByte(const byte val) { write8(&val); }
    inline void writeShort(const short val) { write16(&val); }
    inline void writeInt(const int val) { write32(&val); }
    inline void writeFloat(const float val) { writeFloat(&val); }

    inline BoolIterator writeBoolIt(const bool val) { return writeBoolIt(&val); }
    inline ByteIterator writeByteIt(const byte val) { return write8It(&val); }
    inline ByteIterator writeShortIt(const short val) { return write16It(&val); }
    inline ByteIterator writeIntIt(const int val) { return write32It(&val); }
    inline ByteIterator writeFloatIt(const float val) { return writeFloatIt(&val); }

    inline void writeBoolAt(const bool val, const BoolIterator& it) { write__Bool__At(&val,it); } // Differrent function name, pointer interpretted as bool
    inline void writeByteAt(const byte val, const ByteIterator& it) { write8At(&val,it); }
    inline void writeShortAt(const short val, const ByteIterator& it) { write16At(&val,it); }
    inline void writeIntAt(const int val, const ByteIterator& it) { write32At(&val,it); }
    inline void writeFloatAt(const float val, const ByteIterator& it) { writeFloatAt(&val,it); }

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

    // Read
    inline Buffer& operator >> (bool& val) { readBool(&val); return *this; }
    //
    inline Buffer& operator >> (int8_t& val) { read8(&val); return *this; }
    inline Buffer& operator >> (uint8_t& val) { read8(&val); return *this; }
    //
    inline Buffer& operator >> (int16_t& val) { read16(&val); return *this; }
    inline Buffer& operator >> (uint16_t& val) { read16(&val); return *this; }
    //
    inline Buffer& operator >> (int32_t& val) { read32(&val); return *this; }
    inline Buffer& operator >> (uint32_t& val) { read32(&val); return *this; }
    //
    inline Buffer& operator >> (float& val) { readFloat(&val); return *this; }
    inline Buffer& operator >> (std::string& val) { readString(val); return *this; }

    // By reference argument
    inline void readBool(bool& val) { readBool(&val); }
    inline void readByte(byte& val) { read8(&val); }
    inline void readShort(short& val) { read16(&val); }
    inline void readInt(int& val) { read32(&val); }
    inline void readFloat(float& val) { readFloat(&val); }

    // By return value
    inline bool readBool() { bool val; readBool(&val); return val; }
    inline byte readByte() { byte val; read8(&val); return val; }
    inline short readShort() { short val; read16(&val); return val; }
    inline int readInt() { int val; read32(&val); return val; }
    inline float readFloat() { float val; readFloat(&val); return val; }

    // Peek without increasing the internal iterator
    inline byte peekByte() { byte val; peek8(&val); return val; }
    inline short peekShort() { short val; peek16(&val); return val; }
    inline int peekInt() { int val; peek32(&val); return val; }
    inline float peekFloat() { float val; peekFloat(&val); return val; }
    inline std::string peekString() { return peekString(); }

    // Container helpers
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
    // Those functions shoudn't need to be used by the user.
    // ********************************************************************

    void writeBool(const bool* v);
    BoolIterator writeBoolIt(const bool* v);
    void write__Bool__At(const bool* v, const BoolIterator& it);

    void write8(const void* v);
    ByteIterator write8It(const void* v);
    void write8At(const void* v, const ByteIterator& it);

    void write16(const void* v);
    ByteIterator write16It(const void* v);
    void write16At(const void* v, const ByteIterator& it);

    void write32(const void* v);
    ByteIterator write32It(const void* v);
    void write32At(const void* v, const ByteIterator& it);

    void writeFloat(const float* v);
    ByteIterator writeFloatIt(const float* v);
    void writeFloatAt(const float* v, const ByteIterator& it);

    void writeString(const std::string& v);

    void peek8(void* v);
    void peek16(void* v);
    void peek32(void* v);
    void peekFloat(void* v);
    void peekString(std::string& v);

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
