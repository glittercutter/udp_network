#pragma once

#include "../udpnetwork_Packet.h"
#include <iostream>

namespace udp_network
{


// fwdcl
class ReplicatedVariableContainer;


class ReplicatedVariableBase
{
friend class ReplicatedVariableContainer;

protected:
    ReplicatedVariableBase()
    :   bUpdated(true) {}
    virtual ~ReplicatedVariableBase() {}

    virtual void send(Buffer& stream) = 0;
    virtual void receive(Buffer& stream) = 0;
    
    bool updated() { return bUpdated; }
    void force() { bUpdated = true; }

    bool bUpdated;
};


template <class T>
class ReplicatedVariable : public ReplicatedVariableBase
{
friend class ReplicatedVariableContainer;

public:
    const T& get() { return mData; }
    void set(const T& data)
    {
        if (data == mData) return;
        mData = data;
        bUpdated = true;
    }

protected:
    template <typename ...Args>
    ReplicatedVariable(Args&&... args)
    :   ReplicatedVariableBase(), mData(std::forward<Args>(args)...) {}

    void send(Buffer& stream) { /*stream << mData;*/ bUpdated = false; }
    void receive(Buffer& stream) { /*stream >> mData;*/ }

private:
    T mData;
};


class ReplicatedVariableContainer
{
friend class ReplicatedVariableBase;

public:
    ReplicatedVariableContainer()
    :   bForce(false)
    {
    }
    ~ReplicatedVariableContainer()
    {
        for (auto var : mVariables) delete var;
    }

    template <class T, typename ...Args>
    ReplicatedVariable<T>& add(Args&&... args)
    {
        auto t = new ReplicatedVariable<T>(std::forward<Args>(args)...);
        mVariables.emplace_back(t);
        mReceived.push_back(false);
        return *t;
    }

    template <class STREAM_T>
    void send(STREAM_T& stream)
    {
        // Write updated variable
        for (size_t i = 0; i < mVariables.size(); i++)
        {
            if (mVariables[i]->updated() || bForce) stream << true; 
            else stream << false;
        }

        for (size_t i = 0; i < mVariables.size(); i++)
        {
            if (mVariables[i]->updated() || bForce)
            {
                std::cout<<"replicated updated, sending: "<<i<<std::endl;
                mVariables[i]->send(stream);
            }
        }

        bForce = false;
    }

    template <class STREAM_T>
    void receive(STREAM_T& stream)
    {
        // Read received variable
        for (size_t i = 0; i < mVariables.size(); i++)
        {
            bool b;
            stream >> b;
            mReceived[i] = b;
            std::cout<<"readed updated: "<<i<<" " <<b<<std::endl;
        }

        for (size_t i = 0; i < mVariables.size(); i++)
        {
            if (mReceived[i])
            {
                std::cout<<"replicated updated, received: "<<i<<std::endl;
                mVariables[i]->receive(stream);
            }
        }
    }

    void force() { bForce = true; }

    // Stream operator
    template <class STREAM_T>
    friend STREAM_T& operator >> (STREAM_T& stream, ReplicatedVariableContainer& c)
    {
        c.receive(stream);
        return stream;
    }

    template <class STREAM_T>
    friend STREAM_T& operator << (STREAM_T& stream, ReplicatedVariableContainer& c)
    {
        c.send(stream);
        return stream;
    }

protected:
    bool bForce;
    std::vector<ReplicatedVariableBase*> mVariables;
    std::vector<bool> mReceived;
};


} // namespace udp_network
