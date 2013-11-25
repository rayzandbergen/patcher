/*! \file toggler.h
 *  \brief Contains a Toggler object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TOGGLER_H
#define TOGGLER_H
#include <stdint.h>
#include "mididriver.h"
/*! \brief This class toggles on note on events.
 */
class Toggler
{
    //! \brief Status of a single note.
    enum NoteStatus { On, Hanging, On2, Off };
    bool m_enabled;     //!< Enabled flag.
    NoteStatus m_noteStatus[Midi::Note::max];   //!< Status of every single note.
public:
    //! \brief Return true if enabled.
    bool enabled() const { return m_enabled; }
    Toggler();
    //! \brief Enable the Toggler.
    void enable() { m_enabled = true; }
    //! \brief Return true if ths midi message should be let through.
    bool pass(uint8_t midiStatus, uint8_t data1, uint8_t data2);
    //! \brief Reset the Toggler.
    void reset();
};
#endif
