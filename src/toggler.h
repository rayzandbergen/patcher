/*! \file toggler.h
 *  \brief Contains a Toggler object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TOGGLER_H
#define TOGGLER_H
#include <stdint.h>
#include "midi.h"
/*! \brief This class toggles on note on events.
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
