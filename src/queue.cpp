#include "queue.h"
#include <fcntl.h>
#include <errno.h>
#include "error.h"

void Queue::createWrite()
{
    m_descriptor = mq_open(m_name, O_WRONLY|O_NONBLOCK|O_CREAT, S_IWUSR, 0);
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

Queue::Queue(bool readOnly): m_overruns(0)
{
    m_name = "/patcher_queue";
    if (readOnly)
        createRead();
    else
        createWrite();
}

void Queue::send(const LogMessage &message)
{
    int rv = mq_send(m_descriptor, 
            (const char*)&message, sizeof(message), 0);
    if (rv == EAGAIN)
    {
        m_overruns++;
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
