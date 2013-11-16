/*! \file trackdef.h
 *  \brief Contains objects to handle track definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TRACKDEF_H
#define TRACKDEF_H
#include <vector>
#include <stdint.h>
#include "patcher.h"
#include "monofilter.h"
#include "transposer.h"
#include "controller.h"
#include "toggler.h"
#include "screen.h"

namespace Fantom { 
    class Part;
}
//! \brief Contains some placeholder constants, which are replaced by actual indexes after the TrackList is complete.
namespace TrackDef {
    static const int Unspecified = -1;      //!<    Unspecified track or section.
    static const int PreviousTrack = -2;    //!<    Previous track.
    static const int CurrentTrack = -3;     //!<    Current track.
    static const int NextTrack = -4;        //!<    Next track.
    static const int LastSection = -5;      //!<    Last section of the track.
}

/*! \brief Contains all the parameters needed to manipulate events on a single MIDI channel.
 *
 * This is the counterpart of \a Fantom::Part, although the mapping is not strictly 1-to-1.
 * \todo In some older track definitions, a single \a SwPart may trigger more than one
 * \a Fantom::Part. This should be fixed in tracks.xml.
 */
class SwPart {
public:
    const char *m_name;                 //!< The name of the part. Ownership of the string is assumed.
    uint8_t m_channel;                  //!< MIDI channel to listen and send on.
    int m_transpose;                    //!< Tranposition in semitones.
    bool m_customTransposeEnabled;      //!< Enable switch for custom (per-note) transposition.
    int m_customTranspose[12];          //!< Per-note transposition.
    int m_customTransposeOffset;        //!< Additional transposition for custom transposes.
    uint8_t m_rangeLower;               //!< Lower range.
    uint8_t m_rangeUpper;               //!< Upper range.
    std::vector <const Fantom::Part *> m_hwPart;  //!< List of \a Fantom::Part pointers this will trigger.
    ControllerRemap *m_controllerRemap; //!< Remap object. \todo allow more than one.
    MonoFilter m_monoFilter;            //!< Mono filter object.
    Toggler m_toggler;                  //!< Toggler object.
    bool m_mono;                        //!< Mono toggle.
    Transposer *m_transposer;
    //! \brief Return true if a note number is in range of this \a SwPart.
    bool inRange(uint8_t noteNum) const
        { return noteNum >= m_rangeLower && noteNum <= m_rangeUpper; }
    static uint8_t stringToNoteNum(const char *s);
    SwPart(const char *name, uint8_t channel = 255, int transpose = 0,
            const char *lower = NULL, const char *upper = NULL,
            bool mono=false, Transposer *t = NULL);
    ~SwPart();
    void clear();
    void dumpToLog(Screen *screen, const char *prefix) const;
};

//! \brief A list of \a Section object pointers.
typedef std::vector<SwPart *> SwPartList;

/*! \brief A single keyboard layout.
 *
 * This is a collection of \a SwPart objects, and some section chaining info
 * which specifies what to do on 'next' or 'previous' events.
 */
class Section {
public:
    const char *m_name;             //!< The name of the section. Ownership of the string is assumed.
    bool m_noteOffEnter;            //!< Force 'note off' events when switching to this section.
    bool m_noteOffLeave;            //!< Force 'note off' events when switching away from this section.
    SwPartList m_part;              //!< List of \a SwPart objects.
    int m_nextTrack;                //!< Chaining info: next \a Track.
    int m_nextSection;              //!< Chaining info: next \a Section.
    int m_previousTrack;            //!< Chaining info: previous \a Track.
    int m_previousSection;          //!< Chaining info: previous \a Section.
    Section(const char *name, bool noteOffEnter = true, bool noteOffLeave = true);
    ~Section();
    void clear();
    //! \brief Append \a Swpart to this \a Section.
    void addPart(SwPart *s) { m_part.push_back(s); }
    //! \brief Return the number of parts in this \a Section.
    int nofParts(void) const { return (int)m_part.size(); }
    //! \brief Dump this \a section to a log file.
    void dumpToLog(Screen *screen, const char *prefix) const;
};

//! \brief A list of \a Section object pointers.
typedef std::vector<Section *> SectionList;

/*! \brief The equivalent of a \a FantomPerformance.
 */
class Track {
public:
    const char *m_name;             //!< Name of this track. Ownership of the string is assumed.
    SectionList m_section;   //!< Section list.
    bool m_chain;                   //!< Chain mode switch. If enabled, FCB1010 program changes are interpreted as 'next' and 'previous' events.
    int m_startSection;             //!< Section index to switch to when this track starts.
    Track(const char *name, bool chain = false, int startSection = 0);
    ~Track();
    void clear();
    void addSection(Section *s) { m_section.push_back(s); }
    int nofSections(void) const { return (int)m_section.size(); }
    void dumpToLog(Screen *screen, const char *prefix) const;
};

typedef std::vector<Track*> TrackList;  //!< A list of \a Track object pointers.

//! \brief A setlist object.
class SetList {
    std::vector<int> m_list;    //!< List of track indexes.
public:
    //! \brief Return the length ofthe \a SetList.
    int length(void) const { return m_list.size(); }
    //! \brief Return the track index of the i'th item in the \a SetList.
    int operator[](int i) { return i>=0 && i<length() ? m_list[i] : 0; }
    //! \brief Append a track index to the \a SetList.
    void add(int i) { m_list.push_back(i); }
    //! \brief Append a track by name.
    void add(TrackList &trackList, const char *trackName);
};

void fixChain(TrackList &trackList);
void clear(TrackList &trackList);

#endif // TRACKDEF_H
