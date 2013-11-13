/*! \file monofilter.cpp
 *  \brief Contains an object that enables passing only one MIDI note at a time.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "monofilter.h"
MonoFilter::MonoFilter(): m_sustain(false)
{
    for (int i=0; i<MidiNote::max; i++)
    {
        m_noteOnCount[i] = 0;
        m_ringing[i] = false;
    }
}
void MonoFilter::sustain(bool b)
{
    m_sustain = b;
    if (!b)
    {
        for (int i=0; i<MidiNote::max; i++)
            m_ringing[i] = false;
    }
    else
    {
        for (int i=0; i<MidiNote::max; i++)
            if (m_noteOnCount[i] > 0)
            {
                m_ringing[i] = true;
            }
    }
}
bool MonoFilter::passNoteOn(uint8_t note, uint8_t velo)
{
    if (velo > 0)
    {
        m_noteOnCount[note]++;
        if (m_ringing[note])
        {
            // already ringing, drop
            return false;
        }
        if (m_noteOnCount[note] > 1)
        {
            // multiple, drop
            return false;
        }
        if (m_sustain)
            m_ringing[note] = true;
        return true;
    }
    // note off
    if (m_noteOnCount[note] > 0)
    {
        m_noteOnCount[note]--;
        if (m_noteOnCount[note] == 0)
            return true;
    }
    return true; // we should not get here,
            // but if we do, pass the note off to be sure
}
bool MonoFilter::passNoteOff(uint8_t note, uint8_t velo)
{
    (void)velo;
    return passNoteOn(note,0);
}
