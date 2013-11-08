#ifndef TOGGLER_H
#define TOGGLER_H
#include <stdint.h>
#include "midi.h"
/*
 * This class transposes only when sustain is active.
 */
class Toggler
{
    enum NoteStatus { On, Hanging, On2, Off }; 
    bool m_enabled;
    NoteStatus m_noteStatus[128];
public:
    bool enabled() const { return m_enabled; }
    Toggler();
    void enable() { m_enabled = true; }
    bool pass(uint8_t midiStatus, uint8_t data1, uint8_t data2);
    void reset();
};
#endif
