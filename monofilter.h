#ifndef MONOFILTER_H
#define MONOFILTER_H
#include <stdint.h>
#include "screen.h"
class MonoFilter
{
    uint8_t m_noteOnCount[128];
    uint8_t m_ringing[128];
    bool m_sustain;
    public:
    //Screen *m_screen;
    MonoFilter();
    void sustain(bool b);
    bool passNoteOn(uint8_t note, uint8_t velo);
    bool passNoteOff(uint8_t note, uint8_t velo);
};
#endif
