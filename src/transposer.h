/*! \file transposer.h
 *  \brief Contains an object that transposes only when sustain is active.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TRANSPOSER_H
#define TRANSPOSER_H
#include <stdint.h>
#include "midi.h"
/*! \brief This class transposes only when sustain is active.
 */
class Transposer
{
    bool m_noteState[128];
    bool m_sustain;
    public:
    Transposer(uint8_t offset = 12): m_transpose(offset) { };
    uint8_t m_transpose;
    void setSustain(bool b) { m_sustain = b; };
    void transpose(uint8_t midiStatus, uint8_t &data1, uint8_t &data2);
};
#endif
