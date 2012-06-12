#include "udpnetwork_Packet.h"

using namespace udp_network;

/*
 * Buffer
 */

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

// Should be called after writing any data
void Buffer::addAck(PacketId id)
{
    mData[PacketTypePosition] |= PF_HAS_ACK;
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
    return *(PacketId*)&mData[mData.size()-1 - getAckCount()*sizeof(PacketId)];
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

void Buffer::writeBool(const bool* v)
{
    incrementBool(true);
    *((byte*)&mData[mBoolByteIt]) |= *v << mBoolBitIt++;
    std::cout<<byteToString(mData[mBoolByteIt])<<" bytenum:"<<mBoolByteIt<<std::endl;
}

void Buffer::write8(const void* v)
{
    mData[mByteIt++] = *(const uint8_t*)v;
    mSize = mByteIt;
}

void Buffer::write16(const void* v)
{
    *((Number_t*)&mData[mByteIt]) = *(const uint16_t*)v;
    mByteIt += sizeof(uint16_t);
    mSize = mByteIt;
}

void Buffer::write32(const void* v)
{
    *((Number_t*)&mData[mByteIt]) = *(const uint32_t*)v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
}

void Buffer::writeFloat(const float* v)
{
    *((float*)&mData[mByteIt]) = *v;
    mByteIt += sizeof(Number_t);
    mSize = mByteIt;
}

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
    std::cout<<byteToString(mData[mBoolByteIt])<<" bytenum:"<<mBoolByteIt<<std::endl;
}

void Buffer::read8(void* v)
{
    *(uint8_t*)v = mData[mByteIt++];
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

void Buffer::incrementBool(bool write/* = false*/)
{
    if (mBoolByteIt == InvalidBoolByteIt || mBoolBitIt >= 8)
    {
        mBoolBitIt = 0;
        mBoolByteIt = mByteIt++;
        if (write) mData[mBoolByteIt] = 0;
    }
}

void Buffer::finalize()
{
    if (mNumberOfAck) write8(&mNumberOfAck);
}
