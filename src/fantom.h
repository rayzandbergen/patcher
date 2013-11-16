/*! \file fantom.h
 *  \brief Contains classes to store patches/parts/performances (Roland-speak) from the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef FANTOM_H
#define FANTOM_H
#include <stdint.h>
#include "screen.h"
#include "midi.h"
#include "dump.h"

static const int FantomNameLength = 12;                         //!< Name length of a Fantrom object.
static const int FantomPresetLength = 20;                       //!< Name length of a Fantrom preset.

/*! \brief A Fantom 'patch', i.e. a basic sound.
 *
 * We are only interested in its name, so this is what this class stores.
 */
class FantomPatch
{
public:
    char m_name[FantomNameLength+1];                            //!< The patch name
    void save(const Dump *d) { d->save((const char*)m_name); }  //!< Save the patch to a \a Dump object.
    void restore(const Dump *d) { d->restore((char*)m_name); }  //!< Restore the patch from a \a Dump object.
};

/*! \brief A Fantom 'part', i.e. a patch with some additional mix parameters.
 */
class FantomPart
{
public:
    uint8_t m_number;           //!<    Part number within \a FantomPerformance, 0-15.
    uint8_t m_channel;          //!<    MIDI channel this part listens on.
    uint8_t m_bankSelectMsb;    //!<    Bank select, upper 7 bits.
    uint8_t m_bankSelectLsb;    //!<    Bank select, lower 7 bits.
    uint8_t m_pCh;              //!<    Program change number.
    char m_preset[FantomPresetLength];   //!<    Textual representation of bank select and program change, according to Roland.
    uint8_t m_vol;              //!<    MIDI volume.
    int8_t m_transpose;         //!<    Transpose value, semitones.
    int8_t m_oct;               //!<    Octave transpose.
    uint8_t m_keyRangeLower;    //!<    Lower key range.
    uint8_t m_keyRangeUpper;    //!<    Upper key range.
    uint8_t m_fadeWidthLower;   //!<    Lower fade width.
    uint8_t m_fadeWidthUpper;   //!<    Upper fade width.
    FantomPatch m_patch;        //!<    \a FantomPatch object.
    void constructPreset(bool &patchReadAllowed);
    void save(const Dump *d);
    void restore(const Dump *d);
    void dumpToLog(Screen *screen, const char *prefix) const;
    /*! \brief Constructor */
    FantomPart(): m_number(255), m_channel(255) { }
};

/*! \brief A Fantom 'performance', i.e. a collection of parts in a proper mix.
 */
class FantomPerformance
{
public:
    static const int NofParts = 16;
    char m_name[FantomNameLength+1];    //!< Performance name
    FantomPart m_part[NofParts];        //!< Parts within this \a FantomPerformance.
    void save(const Dump *d);
    void restore(const Dump *d);
};

/*! \brief Class to upload and download Fantom parameters.
 */
struct Fantom
{
    static const uint8_t programChangeChannel = 0x0f;   //!< MIDI channel the Fantom listens on for program changes.
    Screen *m_screen;           //!< A \a Screen object to log to.
    Midi::Driver *m_midi;       //!< A MIDI driver object.
    /* \brief Constructor
     *
     * Constructs an empty Fantom object.
     */
    Fantom(Screen *screen, Midi::Driver *midi): m_screen(screen), m_midi(midi) { }
    void setParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void getParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void setVolume(uint8_t part, uint8_t val);
    void getPartParams(FantomPart *p, int idx);
    void getPatchName(char *s, int idx);
    void getPerfName(char *s);
    void setPerfName(const char *s);
    void setPartName(int part, const char *s);
};
#endif // FANTOM_H
