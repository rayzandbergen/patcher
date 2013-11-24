#ifndef QUEUE_H
#define QUEUE_H
#include <mqueue.h>
#include <stdint.h>

class LogMessage
{
public:
    static const uint8_t Unknown = 255;
    uint8_t m_currentTrack;
    uint8_t m_currentSection;

    uint8_t m_type;
    uint8_t m_track;
    uint8_t m_section;
    uint8_t m_part;
    uint8_t m_midi[3];
};

class Queue
{
    mqd_t   m_descriptor;
    const char *m_name;
public:
    void createWrite();
    void createRead();
    int m_overruns;
    Queue(bool readOnly = true);
    void send(const LogMessage &message);
    void receive(LogMessage &message);
};
#endif // QUEUE_H
