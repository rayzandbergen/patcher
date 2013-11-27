/*! \file queue.h
 *  \brief Contains a \a POSIX queue object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef QUEUE_H
#define QUEUE_H
#include <mqueue.h>
#include <stdint.h>
#include <string>
#include "timestamp.h"

//! \brief A patcher event.
#ifdef NO_STRUCT_PACK
  class Event
#else
  class __attribute__ ((__packed__)) Event
#endif
{
public:
    //! \brief Event type.
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
    static const uint8_t Unknown = 255; //!<    Placeholder value.
    static uint32_t m_sequenceNumber;   //!<    Global sequence number.
    uint16_t m_packetCounter;           //!<    Sequential counter.
    uint8_t m_currentTrack;             //!<    Current track.
    uint8_t m_currentSection;           //!<    Current section.
    uint8_t m_trackIdxWithinSet;        //!<    Setlist index.
    uint8_t m_type;                     //!<    Event type.
    uint8_t m_deviceId;                 //!<    \sa DeviceId.
    uint8_t m_part;                     //!<    Part number.
    uint8_t m_midi[3];                  //!<    MIDI message.
    std::string toString();             //!<    Export to text string.
    std::string toMidiString();         //!<    Export MIDI message to text string.
    //! \brief Construct a new Event with its packet counter filled in.
    Event(): m_packetCounter((uint16_t)m_sequenceNumber)
    {
        m_sequenceNumber++;
    }
};

//! \brief A POSIX message queue to hold Events.
class Queue
{
#ifdef DOXYGEN // fake this connection, so Doxygen cann see we're queueing Events.
    Event   m_posixQueue[2]; //!<    Event queue.
#endif
    mqd_t   m_descriptor;   //!<    Queue descriptor.
    const char *m_name;     //!<    Resource name.
    void createWrite();     //!<    Create queue for writing.
    void createRead();      //!<    Create queue for reading.
public:
    static const bool Read = true;      //!<    Inidiate Read queue.
    static const bool Write = false;    //!<    Indicate Write queue.
    int m_overruns;                     //!<    Overrun counter.
    Queue(bool readWrite = Read);
    void send(const Event &event);
    void receive(Event &event);
    bool receive(Event &event, const TimeSpec &absTime);
    std::string getAttr();
};
#endif // QUEUE_H
