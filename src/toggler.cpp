#include <stdint.h>
#include "transposer.h"
#include "toggler.h"
bool Toggler::pass(uint8_t midiStatus, uint8_t data1, uint8_t data2)
{
    if (!m_enabled)
        return true;

    midiStatus &= 0xf0;
    data1 &= 127;

    if (!Midi::isNote(midiStatus))
    {
        return true;
    }
    bool noteOn = Midi::isNoteOn(midiStatus, data1, data2);
    bool noteOff = Midi::isNoteOff(midiStatus, data1, data2);
    switch (m_noteStatus[data1])
    {
        case Off:
            if (noteOn)
            {
                m_noteStatus[data1] = On;
            }
            return true;
        case On:
            if (noteOff)
            {
                m_noteStatus[data1] = Hanging;
                return false;
            }
            return true;
        case Hanging:
            if (noteOn)
            {
                m_noteStatus[data1] = On2;
                return false;
            }
            return true;
        case On2:
            if (noteOff)
            {
                m_noteStatus[data1] = Off;
                return true;
            }
            return true;
        default:
            /* we should not be here */
            return false;
    }
}

Toggler::Toggler(): m_enabled(false)
{
    reset();
}

void Toggler::reset()
{
    for (int i=0; i<128; i++)
        m_noteStatus[i] = Off;
}
