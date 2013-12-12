/*! \file fantomdef.h
 *  \brief Contains classes to store patches/parts/performances (Roland-speak) from the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef FANTOM_DEF_H
#define FANTOM_DEF_H
#include <stdint.h>
#include <vector>
#include "screen.h"
#include "mididef.h"
#include "mididriver.h"
#include "dump.h"

class XML;

//! \brief Namespace for Fantom driver objects.
namespace Fantom
{

static const uint8_t programChangeChannel = 0x0f;         //!< MIDI channel the Fantom listens on for program changes.
static const int NameLength = 12;                         //!< Name length of a Fantom patch or performance.
static const int PresetLength = 20;                       //!< Name length of a Fantom preset.

/*! \brief A Fantom 'patch', i.e. a basic sound.
 *
 * The patcher only interested in its name, so this is what this class stores.
 */
class Patch
{
public:
    char m_name[NameLength+1];                            //!< The patch name
    void save(const Dump *d) { d->save((const char*)m_name); }  //!< Save the patch to a \a Dump object.
    void restore(const Dump *d) { d->restore((char*)m_name); }  //!< Restore the patch from a \a Dump object.
    Patch();
};

/*! \brief A Fantom 'part', i.e. a patch with some additional mix parameters.
 *
 *  All integer widths are specified so the binary dumps are portable.
 */
class Part
{
public:
    uint8_t m_channel;          //!<    MIDI channel this part listens on.
    uint8_t m_bankSelectMsb;    //!<    Bank select, upper 7 bits.
    uint8_t m_bankSelectLsb;    //!<    Bank select, lower 7 bits.
    uint8_t m_programChange;    //!<    Program change number.
    char m_preset[PresetLength];//!<    Textual representation of bank select and program change, according to Roland.
    uint8_t m_volume;           //!<    MIDI volume.
    int8_t m_transpose;         //!<    Transpose value, semitones.
    int8_t m_octave;            //!<    Octave transpose.
    uint8_t m_keyRangeLower;    //!<    Lower key range.
    uint8_t m_keyRangeUpper;    //!<    Upper key range.
    uint8_t m_fadeWidthLower;   //!<    Lower fade width.
    uint8_t m_fadeWidthUpper;   //!<    Upper fade width.
    Patch m_patch;              //!<    \a Patch object.
    void constructPreset(bool &patchReadAllowed);
    void save(const Dump *d);
    void restore(const Dump *d);
    void toTextFile(FILE *fp, const char *prefix) const;
    /*! \brief Constructor */
    Part(): m_channel(255) { }
};

/*! \brief A Fantom performance.
 *
 * This object holds all the relevant parameters of a Fantom performance,
 * i.e. a collection of 16 Fantom parts, in a proper mix.
 * At startup, these parameters are downloaded from the Fantom,
 * or from a cache.
 */
class Performance
{
public:
    static const int NofParts = 16; //!< Number of Parts in a Fantom performance.
    char m_name[NameLength+1];    //!< Performance name.
    Part m_partList[NofParts];    //!< Parts within this \a FantomPerformance.
    void save(const Dump *d);
    void restore(const Dump *d);
    Performance();
};

//! \brief List of Performance pointers.
typedef std::vector<Performance*> PerformanceList;

} // Fantom namespace
#endif // FANTOM_DEF_H
