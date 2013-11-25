#ifndef QUEUE_H
#define QUEUE_H
#include <mqueue.h>
#include <stdint.h>
#include <string>

class __attribute__ ((__packed__)) LogMessage
{
public:
    enum Type
    {
        Ready = 0,
        MidiOut1Byte,
        MidiOut2Bytes,
        MidiOut3Bytes,
        MidiIn1Byte,
        MidiIn2Bytes,
        MidiIn3Bytes
    };
    static const uint8_t Unknown = 255;
    static uint32_t m_sequenceNumber;
    uint16_t m_packetCounter;
    uint8_t m_currentTrack;
    uint8_t m_currentSection;
    uint8_t m_type;
    uint8_t m_deviceId;
    uint8_t m_part;
    uint8_t m_midi[3];
    std::string toString();
    LogMessage(): m_packetCounter((uint16_t)m_sequenceNumber)
    {
        m_sequenceNumber++;
    }
};

class Queue
{
    mqd_t   m_descriptor;
    const char *m_name;
    void createWrite();
    void createRead();
public:
    static const bool Read = true;
    static const bool Write = false;
    int m_overruns;
    Queue(bool readWrite = Read);
    void send(const LogMessage &message);
    void receive(LogMessage &message);
    std::string getAttr();
};
#endif // QUEUE_H
