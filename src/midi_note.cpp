#include <stdint.h>
#include <stdio.h>
#include "midi_note.h"

namespace Midi
{

/*! \brief Return true if given status byte is MIDI note data.
 *
 * \param[in]   status  status byte to be checked.
 * \return      true if given status byte id MIDI note data.
 */
bool isNote(uint8_t status)
{
    status &= 0xf0;
    return status == noteOff
        || status == noteOn
        || status == aftertouch;
}

/*! \brief Return true if input is a MIDI note on message.
 *
 * \param[in]   status  status byte to be checked.
 * \param[in]   data1   not used.
 * \param[in]   data2   MIDI data 2.
 * \return      true if given message is a MIDI note on message.
 */
bool isNoteOn(uint8_t status, uint8_t data1, uint8_t data2)
{
    (void)data1;
    status &= 0xf0;
    return status == noteOn && data2 != 0;
}

/*! \brief Return true if input is a MIDI note off message.
 *
 * \param[in]   status  status byte to be checked.
 * \param[in]   data1   not used.
 * \param[in]   data2   MIDI data 2.
 * \return      true if given message is a MIDI note off message.
 */
bool isNoteOff(uint8_t status, uint8_t data1, uint8_t data2)
{
    (void)data1;
    status &= 0xf0;
    return (status == noteOn && data2 == 0)
         || status == noteOff;
}

/*! \brief Return true if input status is a MIDI controller.
 *
 * \param[in]   status  status byte to be checked.
 * \return      true if given status is a MIDI controller.
 */
bool isController(uint8_t status)
{
    status &= 0xf0;
    return status == controller;
}

/*! \brief Convert a note number to a string.
 *
 *  \param[in] num   MIDI note number.
 *  \param[out] s    string buffer, must be at least 5 bytes.
 */
void noteName(uint8_t num, char *s)
{
    const char *str[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    int oct = num / 12;
    num = num % 12;
    sprintf(s, "%s%d", str[num], oct);
}

/*! \brief Convert a note number to a string in a static buffer.
 *
 *  \param[in] num   MIDI note number.
 *  \return static string buffer.
 */
const char *noteName(uint8_t num)
{
    static char buf[5];
    noteName(num, buf);
    return buf;
}

} // namespace Midi
