#include "queue.h"
#include <fcntl.h>
#include <errno.h>
#include "error.h"
#include <sstream>

uint32_t LogMessage::m_sequenceNumber = 0;

void Queue::createWrite()
{
    struct mq_attr attr;
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(LogMessage);
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

Queue::Queue(bool readWrite): m_overruns(0)
{
    m_name = "/patcher_queue";
    if (readWrite == Read)
        createRead();
    else
        createWrite();
}

void Queue::send(const LogMessage &message)
{
    int rv = mq_send(m_descriptor, 
            (const char*)&message, sizeof(message), 0);
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

void Queue::receive(LogMessage &message)
{
    ssize_t rv = mq_receive(m_descriptor, 
            (char *)&message, sizeof(message), 0);
    if (rv < 0)
    {
        throw(Error("mq_receive", errno));
    }
}

bool Queue::receive(LogMessage &message, const TimeSpec &absTime)
{
    ssize_t rv = mq_timedreceive(m_descriptor, 
            (char *)&message, sizeof(message), 0, &absTime);
    if (rv < 0)
    {
        if (errno == ETIMEDOUT)
            return false;
        throw(Error("mq_receive", errno));
    }
    return true;
}

std::string LogMessage::toString()
{
    std::stringstream ss;
    ss.setf(std::ios::hex, std::ios::basefield);
    ss.setf(std::ios::adjustfield, std::ios::right);
    //ss.setf(std::ios::showbase); // completely useless when combined with 'fill'
    ss << "0x"; ss.width(4); ss.fill('0');
    ss << (int)m_packetCounter << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_currentTrack << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_currentSection << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_type << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_deviceId << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_part << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[0] << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[1] << " ";
    ss << "0x"; ss.width(2); ss.fill('0');
    ss << (int)m_midi[2];
    return ss.str();
}

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
