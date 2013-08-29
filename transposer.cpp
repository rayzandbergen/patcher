#include <stdint.h>
#include "transposer.h"
#include "patcher.h"
void Transposer::transpose(uint8_t midiStatus, uint8_t &data1, uint8_t &data2)
{
    midiStatus &= 0xf0;
    data1 &= 127;

    if (!Midi::isNote(midiStatus))
        return;
    if (Midi::isNoteOn(midiStatus, data1, data2))
    {
        m_noteState[data1] = m_sustain;
        if (m_sustain)
        {
            data1 = (data1 + m_transpose) & 127;
        }
    }
    else if (Midi::isNoteOff(midiStatus, data1, data2))
    {
        if (m_noteState[data1])
        {
            data1 = (data1 + m_transpose) & 127;
        }
    }
}
