/*! \file stdoutclient.cpp
 *  \brief Passes events from the event queue to stdout.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdio.h>
#include "queue.h"
#include "error.h"

//! \brief Main entry point, no parameters.
int main()
{
    Queue m_eventRxQueue;
    m_eventRxQueue.openRead();
    Event event;
    for (;;)
    {
        m_eventRxQueue.receive(event);
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
    return 0;
}
