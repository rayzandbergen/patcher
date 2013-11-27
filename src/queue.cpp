/*! \file queue.cpp
 *  \brief Contains a \a POSIX queue object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "queue.h"
#include <fcntl.h>
#include <errno.h>
#include "error.h"
#include <sstream>

uint32_t Event::m_sequenceNumber = 0;

void Queue::createWrite()
{
    struct mq_attr attr;
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Event);
    m_descriptor = mq_open(m_name, O_WRONLY|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &attr);
    if (m_descriptor == -1)
    {
        throw(Error("mq_open O_WRONLY|O_NONBLOCK|O_CREAT", errno));
    }
}

void Queue::createRead()
{
    m_descriptor = mq_open(m_name, O_RDONLY, S_IRUSR, 0);
    if (m_descriptor == -1)
    {
        throw(Error("mq_open O_RDONLY", errno));
    }
}

//! \brief Construct a Queue, 'read' by default.
Queue::Queue(bool readWrite): m_overruns(0)
{
    m_name = "/patcher_queue";
    if (readWrite == Read)
        createRead();
    else
        createWrite();
}

//! \brief Send an Event.
void Queue::send(const Event &event)
{
    int rv = mq_send(m_descriptor,
            (const char*)&event, sizeof(event), 0);
    if (rv == -1)
    {
        if (errno == EAGAIN)
            m_overruns++;
        else
        {
            throw(Error("mq_send", errno));
        }
    }
}

/*! \brief Receive an event.
 * \param[out]  event     The event.
 */
void Queue::receive(Event &event)
{
    ssize_t rv = mq_receive(m_descriptor,
            (char *)&event, sizeof(event), 0);
    if (rv < 0)
    {
        throw(Error("mq_receive", errno));
    }
}

/*! \brief Receive an event.
 *
 * \param[out]  event       The event.
 * \param[in]   absTime     Expiration time.
 * \return true if reception succeeds, false if expired.
 */
bool Queue::receive(Event &event, const TimeSpec &absTime)
{
    ssize_t rv = mq_timedreceive(m_descriptor,
            (char *)&event, sizeof(event), 0, &absTime);
    if (rv < 0)
    {
        if (errno == ETIMEDOUT)
            return false;
        throw(Error("mq_receive", errno));
    }
    return true;
}

std::string Event::toMidiString()
{
    std::stringstream ss;
    ss.setf(std::ios::hex, std::ios::basefield);
    ss.setf(std::ios::adjustfield, std::ios::right);
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[0] << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[1] << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[2];
    return ss.str();
}

std::string Event::toString()
{
    std::stringstream ss;
    ss.setf(std::ios::hex, std::ios::basefield);
    ss.setf(std::ios::adjustfield, std::ios::right);
    //ss.setf(std::ios::showbase); // completely useless when combined with 'fill'
    ss << "0x"; ss.width(4); ss.fill('0');
    ss << (int)m_packetCounter << " ";
    ss << "M0x"; ss.width(2); ss.fill('0');
    ss << (int)m_metaMode << " ";
    ss << "T0x"; ss.width(2); ss.fill('0');
    ss << (int)m_currentTrack << " ";
    ss << "S0x"; ss.width(2); ss.fill('0');
    ss << (int)m_currentSection << " ";
    ss << "L0x"; ss.width(2); ss.fill('0');
    ss << (int)m_trackIdxWithinSet << " ";
    switch(m_type)
    {
        case Ready:
            ss << "Ready";
            break;
        case MidiOut1Byte:
            ss << "MidiOut1Byte";
            break;
        case MidiOut2Bytes:
            ss << "MidiOut2Bytes";
            break;
        case MidiOut3Bytes:
            ss << "MidiOut3Bytes";
            break;
        case MidiIn1Byte:
            ss << "MidiIn1Byte";
            break;
        case MidiIn2Bytes:
            ss << "MidiIn2Bytes";
            break;
        case MidiIn3Bytes:
            ss << "MidiIn3Bytes";
            break;
        default:
            ss << "Unknown Type 0x";
            ss.width(2); ss.fill('0');
            ss << (int)m_type;
            break;
    }
    ss << " D0x"; ss.width(2); ss.fill('0');
    ss << (int)m_deviceId << " ";
    ss << "P0x"; ss.width(2); ss.fill('0');
    ss << (int)m_part << " ";
    ss << toMidiString();
    return ss.str();
}

//! \brief Get some info on the Queue.
std::string Queue::getAttr()
{
    std::stringstream ss;
    struct mq_attr attr;
    mq_getattr(m_descriptor, &attr);
    ss << "flags = " << attr.mq_flags <<
          ", maxmsg = " << attr.mq_maxmsg <<
          ", msgsize = " << attr.mq_msgsize;
    return ss.str();
}
