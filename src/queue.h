#ifndef QUEUE_H
#define QUEUE_H
#include <mqueue.h>
#include <stdint.h>
#include <string>

class LogMessage
{
public:
    enum Type
    {
        MidiOut1Byte = 1,
        MidiOut2Bytes = 2,
        MidiOut3Bytes = 3
    };
    static const uint8_t Unknown = 255;
    uint8_t m_currentTrack;
    uint8_t m_currentSection;

    Type m_type;
    uint8_t m_part;
    uint8_t m_midi[3];
    std::string toString();
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
    std::string getAttr();
};
#endif // QUEUE_H
