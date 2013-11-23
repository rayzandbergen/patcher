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
};

/*! \brief A Fantom 'part', i.e. a patch with some additional mix parameters.
 */
class Part
{
public:
    uint8_t m_number;           //!<    Part number within \a Performance, 0-15.
    uint8_t m_channel;          //!<    MIDI channel this part listens on.
    uint8_t m_bankSelectMsb;    //!<    Bank select, upper 7 bits.
    uint8_t m_bankSelectLsb;    //!<    Bank select, lower 7 bits.
    uint8_t m_pCh;              //!<    Program change number.
    char m_preset[PresetLength];//!<    Textual representation of bank select and program change, according to Roland.
    uint8_t m_vol;              //!<    MIDI volume.
    int8_t m_transpose;         //!<    Transpose value, semitones.
    int8_t m_oct;               //!<    Octave transpose.
    uint8_t m_keyRangeLower;    //!<    Lower key range.
    uint8_t m_keyRangeUpper;    //!<    Upper key range.
    uint8_t m_fadeWidthLower;   //!<    Lower fade width.
    uint8_t m_fadeWidthUpper;   //!<    Upper fade width.
    Patch m_patch;              //!<    \a Patch object.
    void constructPreset(bool &patchReadAllowed);
    void save(const Dump *d);
    void restore(const Dump *d);
    void dumpToLog(Screen *screen, const char *prefix) const;
    /*! \brief Constructor */
    Part(): m_number(255), m_channel(255) { }
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
};

/*! \brief Upload and download parameters from and to the Fantom.
 */
struct Driver
{
    WINDOW *m_window;
    Midi::Driver *m_midi;       //!< A MIDI driver object.
    /* \brief Constructor
     *
     * Constructs an empty Driver object.
     */
    //! \brief Contruct a default Driver object.
    Driver(WINDOW *window, Midi::Driver *midi): m_window(window), m_midi(midi) { }
    void setParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void getParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void setVolume(uint8_t part, uint8_t val);
    void getPartParams(Part *p, int idx);
    void getPatchName(char *s, int idx);
    void getPerfName(char *s);
    void setPerfName(const char *s);
    void setPartName(int part, const char *s);
    void selectPerformance(uint8_t performanceIndex) const;
    void selectPerformanceFromMemCard() const;
};

class PerformanceList
{    
    static const uint32_t magic = 0x46a28f12;
    Fantom::Performance *m_performanceList;
    uint32_t m_magic;
    size_t m_size;
public:
    size_t size() const { return m_size; }
    void clear();
    PerformanceList(): m_performanceList(0) { }
    Fantom::Performance *operator[](size_t index) const
    {
        return &m_performanceList[index];
    }
    size_t readFromCache(const char *fantomPatchFile);
    void writeToCache(const char *fantomPatchFile);
    void download(Fantom::Driver *fantom, WINDOW *win, size_t nofTracks);
};

} // Fantom namespace
#endif // FANTOM_H
