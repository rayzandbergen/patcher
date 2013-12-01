/*! \file monofilter.h
 *  \brief Contains an object that enables passing only one MIDI note at a time.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef MONOFILTER_H
#define MONOFILTER_H
#include <stdint.h>
#include "screen.h"
#include "mididef.h"
/*! \brief This class prevents re-triggering of notes while they sound.
 */
class MonoFilter
{
    uint8_t m_noteOnCount[Midi::Note::max];     //!< Number of note on events seen for each note.
    bool m_ringing[Midi::Note::max];            //!< 'ringing' flag for each note.
    bool m_sustain;                             //!< Last known state of the sustain pedal.
public:
    MonoFilter();
    void sustain(bool b);
    bool passNoteOn(uint8_t note, uint8_t velo);
    bool passNoteOff(uint8_t note, uint8_t velo);
};
#endif
