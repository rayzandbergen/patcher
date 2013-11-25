/*! \file transposer.h
 *  \brief Contains an object that transposes only when sustain is active.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TRANSPOSER_H
#define TRANSPOSER_H
#include <stdint.h>
#include "mididef.h"
#include "mididriver.h"
/*! \brief This class transposes only when sustain is active.
 */
class Transposer
{
    bool m_noteState[Midi::Note::max];  //!<    Sustain status for each MIDI note.
    bool m_sustain;                     //!<    Latest sustain pedal status.
public:
    uint8_t m_transpose;                //!<    Transpose value in semitones.
    //! \brief Construct a transposer that by default transposes one octave.
    Transposer(uint8_t offset = 12): m_transpose(offset) { };
    //! \brief Set the sustain pedal status.
    void setSustain(bool b) { m_sustain = b; };
    void transpose(uint8_t midiStatus, uint8_t &data1, uint8_t &data2);
};
#endif
