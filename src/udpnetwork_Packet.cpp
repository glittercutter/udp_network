#include "udpnetwork_Packet.h"
#include <stdexcept>

using namespace udp_network;


#define UDP_NETWORK_CHECK_BUFFER_OVERFLOW(_TYPE) \
{ \
    if (mSize + sizeof(_TYPE) >= mData.size()-1) throw std::runtime_error("UDPNETWORK buffer overflow!"); \
} 


/*
 * Buffer
 */

void Buffer::eraseLastByte()
{
    --mByteIt;
    mSize = mByteIt;
}

void Buffer::eraseLastShort()
{
    mByteIt -= sizeof(int16_t);
    mSize = mByteIt;
}

void Buffer::eraseLastInt()
{
    mByteIt -= sizeof(int32_t);
    mSize = mByteIt;
}

void Buffer::eraseLastFloat()
{
    mByteIt -= sizeof(float);
    mSize = mByteIt;
}

void Buffer::setReliable(bool state)
{
    if (state) mData[PacketTypePosition] |= PF_RELIABLE;
    else mData[PacketTypePosition] &= ~PF_RELIABLE;
}

bool Buffer::getReliable() const
{
    return mData[PacketTypePosition] & PF_RELIABLE;
}

void Buffer::setType(byte t)
{
    // Extract flag, then write type and flag
    byte flags = mData[PacketTypePosition] >> ((sizeof(PacketType) * 8) - PacketFlagCount);
    mData[PacketTypePosition] = t | flags;
}

byte Buffer::getType() const
{
    // Clear flag to leave only the type;
    byte b = mData[PacketTypePosition] << PacketFlagCount;
    return b >> PacketFlagCount;
}

void Buffer::setId(PacketId id)
{
    *(PacketId*)&mData[PacketIdPosition] = id;
}

PacketId Buffer::getId() const
{
    return *(PacketId*)&mData[PacketIdPosition];
}

// Should be called after the data writing stage
void Buffer::addAck(PacketId id)
{
    write16(&id);   
    ++mNumberOfAck;
}

byte Buffer::getAckCount()
{
    if (hasAck()) return mData[mSize-1];
    return 0;
}

bool Buffer::hasAck()
{
    return mData[PacketTypePosition] & PF_HAS_ACK;
}

PacketId Buffer::getAck(byte i)
{
    return *(PacketId*)&mData[mSize-1 - getAckCount()*sizeof(PacketId) + i*sizeof(PacketId)];
}

void Buffer::clear()
{
    mData[0] = 0;
    mSize = PacketHeaderSize;
    mByteIt = PacketHeaderSize;
    mBoolByteIt = InvalidBoolByteIt;
    mBoolBitIt = 0;
    mNumberOfAck = 0;
}

std::string byteToString(byte b)
{
    std::stringstream ss;

    for (int i = 7; i >= 0; i--)
        if (b & (1 << i)) ss << 1;
        else ss << 0;

    return ss.str();
}

std::string Buffer::debugHeader()
{
    std::stringstream ss;
    ss << "info: " << byteToString(mData[PacketTypePosition]);
    ss << "\nid: " << *(PacketId*)&mData[PacketIdPosition];
    return ss.str();
}

//

void Buffer::writeBool(const bool* v)
{
    incrementBool(true);
    *((byte*)&mData[mBoolByteIt]) |= *v << mBoolBitIt++;
}

Buffer::BoolIterator Buffer::writeBoolIt(const bool* v)
{
    incrementBool(true);
    BoolIterator it(mByteIt,mBoolBitIt);
    *((byte*)&mData[mBoolByteIt]) |= *v << mBoolBitIt++;
    return it;
}

void Buffer::write__Bool__At(const bool* v, const BoolIterator& it)
{
    *((byte*)&mData[it.BytePosition]) |= *v << it.BitPosition;
}

//

void Buffer::write8(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint8_t);
    mData[mByteIt++] = *(const uint8_t*)v;
    mSize = mByteIt;
}

Buffer::ByteIterator Buffer::write8It(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint8_t);
    ByteIterator it(mByteIt);
    mData[mByteIt++] = *(const uint8_t*)v;
    mSize = mByteIt;
    return it;
}

void Buffer::write8At(const void* v, const ByteIterator& it)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint8_t);
    mData[it.BytePosition] = *(const uint8_t*)v;
}

//

void Buffer::write16(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint16_t);
    *((Number_t*)&mData[mByteIt]) = *(const uint16_t*)v;
    mByteIt += sizeof(uint16_t);
    mSize = mByteIt;
}

Buffer::ByteIterator Buffer::write16It(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint16_t);
    ByteIterator it(mByteIt);
    *((Number_t*)&mData[mByteIt]) = *(const uint16_t*)v;
    mByteIt += sizeof(uint16_t);
    mSize = mByteIt;
    return it;
}

void Buffer::write16At(const void* v, const ByteIterator& it)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint16_t);
    *((Number_t*)&mData[it.BytePosition]) = *(const uint16_t*)v;
}

//

void Buffer::write32(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint32_t);
    *((Number_t*)&mData[mByteIt]) = *(const uint32_t*)v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
}

Buffer::ByteIterator Buffer::write32It(const void* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint32_t);
    ByteIterator it(mByteIt);
    *((Number_t*)&mData[mByteIt]) = *(const uint32_t*)v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
    return it;
}

void Buffer::write32At(const void* v, const ByteIterator& it)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(uint32_t);
    *((Number_t*)&mData[it.BytePosition]) = *(const uint32_t*)v;
}

//

void Buffer::writeFloat(const float* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(float);
    *((float*)&mData[mByteIt]) = *v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
}

Buffer::ByteIterator Buffer::writeFloatIt(const float* v)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(float);
    ByteIterator it(mByteIt);
    *((float*)&mData[mByteIt]) = *v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
    return it;
}

void Buffer::writeFloatAt(const float* v, const ByteIterator& it)
{
    UDP_NETWORK_CHECK_BUFFER_OVERFLOW(float);
    *((float*)&mData[it.BytePosition]) = *v;
}

//

void Buffer::writeString(const std::string& v)
{
    memcpy((char*)&mData[mByteIt], v.c_str(), v.size() + 1);
    mByteIt += v.size() + 1;
    mSize = mByteIt;
}

void Buffer::readBool(bool* v)
{
    incrementBool();
    *v = (mData[mBoolByteIt] >> mBoolBitIt++) & 1;
}

void Buffer::read8(void* v)
{
    *(uint8_t*)v = mData[mByteIt++];
}

void Buffer::read16(void* v)
{
    *(uint16_t*)v = mData[mByteIt];
    mByteIt += sizeof(uint16_t);
}

void Buffer::read32(void* v)
{
    *(uint32_t*)v = *(uint32_t*)&mData[mByteIt];
    mByteIt += sizeof(uint32_t);
}

void Buffer::readFloat(float* v)
{
    *v = *(float*)&mData[mByteIt];
    mByteIt += sizeof(Number_t);
}

void Buffer::readString(std::string& v)
{
    v = (char*)&mData[mByteIt];
    mByteIt += v.size() + 1;
}

void Buffer::peek8(void* v)
{
    *(uint8_t*)v = mData[mByteIt];
}

void Buffer::peek16(void* v)
{
    *(uint16_t*)v = mData[mByteIt];
}

void Buffer::peek32(void* v)
{
    *(uint32_t*)v = *(uint32_t*)&mData[mByteIt];
}

void Buffer::peekFloat(void* v)
{
    *(float*)v = *(float*)&mData[mByteIt];
}

void Buffer::peekString(std::string& v)
{
    v = (char*)&mData[mByteIt];
}

void Buffer::incrementBool(bool write/* = false*/)
{
    if (mBoolByteIt == InvalidBoolByteIt || mBoolBitIt >= 8)
    {
        mBoolBitIt = 0;
        mBoolByteIt = mByteIt++;
        mSize = mByteIt;
        if (write) mData[mBoolByteIt] = 0;
    }
}

void Buffer::finalize()
{
    if (mNumberOfAck)
    {
        mData[PacketTypePosition] |= PF_HAS_ACK;
        write8(&mNumberOfAck);
    }
}

bool Buffer::eof()
{
    return mByteIt >= mSize-1 - 1 - getAckCount()*sizeof(PacketId);
}
