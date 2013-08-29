#include "monofilter.h"
MonoFilter::MonoFilter(): m_sustain(false)
{
    for (int i=0; i<128; i++)
    {
        m_noteOnCount[i] = 0;
        m_ringing[i] = 0;
    }
}
void MonoFilter::sustain(bool b)
{
    m_sustain = b;
    if (!b)
    {
        for (int i=0; i<128; i++)
            m_ringing[i] = 0;
    }
    else
    {
        for (int i=0; i<128; i++)
            if (m_noteOnCount[i] > 0)
            {
                m_ringing[i] = 1;
                //wprintw(m_screen->m_midiLog, "ringing %02x\n", i);
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
            //wprintw(m_screen->m_midiLog, "ringing %02x, drop\n", note);
            return false;
        }
        if (m_noteOnCount[note] > 1)
        {
            //wprintw(m_screen->m_midiLog, "multiple %02x, drop\n", note);
            return false;
        }
        if (m_sustain)
            m_ringing[note] = 1;
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
