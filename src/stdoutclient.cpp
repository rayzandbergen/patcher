/*! \file stdoutclient.cpp
 *  \brief Passes events from the event queue to stdout.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdio.h>
#include "queue.h"
#include "mididef.h"
#include "mididriver.h"
#include "error.h"

void printEvent(const Event &event)
{
    printf("%04x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        event.m_packetCounter,
        event.m_metaMode,
        event.m_currentTrack,
        event.m_currentSection,
        event.m_trackIdxWithinSet,
        event.m_type,
        event.m_deviceId,
        event.m_part,
        event.m_midi[0],
        event.m_midi[1],
        event.m_midi[2]);
    fflush(stdout);
}

void fakeEvents()
{
    Event event;
    for (int i=0;; i++)
    {
        event.m_packetCounter = i;
        event.m_metaMode = i % 10 == 0;
        event.m_currentTrack = i % 10 < 4;
        event.m_currentSection = i % 2;
        event.m_trackIdxWithinSet = i / 15;
        event.m_type = Event::MidiOut3Bytes;
        event.m_deviceId = Midi::Device::FantomOut;
        event.m_part = 0;
        if (i % 2 == 0)
        {
            event.m_midi[0] = Midi::noteOn;
            event.m_midi[1] = Midi::Note::C4;
            event.m_midi[2] = 0x45 + i%3;
        }
        else
        {
            event.m_midi[0] = Midi::noteOn;
            event.m_midi[1] = Midi::Note::C4;
            event.m_midi[2] = 0;
        }
        printEvent(event);
        sleep(1);
    }
}

void passEvents()
{
    Queue m_eventRxQueue;
    m_eventRxQueue.openRead();
    Event event;
    for (;;)
    {
        m_eventRxQueue.receive(event);
        printEvent(event);
    }
}

//! \brief Main entry point, no parameters.
int main()
{
    passEvents();
    //fakeEvents();
    return 0;
}
