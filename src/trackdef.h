/*! \file trackdef.h
 *  \brief Contains objects to handle track definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TRACKDEF_H
#define TRACKDEF_H
#include <vector>
#include <stdint.h>
#include "patchercore.h"
#include "monofilter.h"
#include "transposer.h"
#include "controller.h"
#include "toggler.h"
#include "screen.h"

#ifdef FAKE_STL // set in PREDEFINED in doxygen config
namespace std { /*! \brief STL vector */ template <class T> class vector {
        public T entry[2]; /*!< Entry. */ }; }
#endif

namespace Fantom {
    class Part;
    class Performance;
}
//! \brief Contains some placeholder chaining constants, which are replaced by actual indexes after the TrackList is complete.
namespace TrackDef {
    //! \brief Placeholder constants.
    enum {
        Unspecified = -1,       //!<    Unspecified Track or Section.
        PreviousTrack = -2,     //!<    Previous track.
        CurrentTrack = -3,      //!<    Current track.
        NextTrack = -4,         //!<    Next track.
        LastSection = -5        //!<    Lastsection of the track.
    };
}

/*! \brief Contains all the parameters needed to manipulate events on a single MIDI channel.
 *
 * This is the counterpart of \a Fantom::Part, although a single SwPart may trigger more than one \a Fantom::Part. By convention there should be a single Fantom::Part on each channel.
 */
class SwPart {
public:
    int m_number;                       //!< Sequence number within Section.
    const char *m_name;                 //!< The name of the part. Ownership of the string is assumed.
    uint8_t m_channel;                  //!< MIDI channel to listen and send on.
    int m_transpose;                    //!< Tranposition in semitones.
    bool m_customTransposeEnabled;      //!< Enable switch for custom (per-note) transposition.
    int m_customTranspose[12];          //!< Per-note transposition.
    int m_customTransposeOffset;        //!< Additional transposition for custom transposes.
    uint8_t m_rangeLower;               //!< Lower range.
    uint8_t m_rangeUpper;               //!< Upper range.
    std::vector <Fantom::Part *> m_hwPartList;   //!< List of \a Fantom::Part pointers this will trigger.
    ControllerRemap::Default *m_controllerRemap; //!< Remap object. \todo allow more than one.
    MonoFilter m_monoFilter;            //!< Mono filter object.
    Toggler m_toggler;                  //!< Toggler object.
    bool m_mono;                        //!< Mono toggle.
    Transposer *m_transposer;           //!< Transpose object.
    //! \brief Return true if a note number is in range of this \a SwPart.
    bool inRange(uint8_t noteNum) const
        { return noteNum >= m_rangeLower && noteNum <= m_rangeUpper; }
    SwPart(int number, const char *name);
    ~SwPart();
    void clear();
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
    SwPartList m_partList;          //!< List of \a SwPart objects.
    int m_nextTrack;                //!< Chaining info: next \a Track.
    int m_nextSection;              //!< Chaining info: next \a Section.
    int m_previousTrack;            //!< Chaining info: previous \a Track.
    int m_previousSection;          //!< Chaining info: previous \a Section.
    Section(const char *name);
    ~Section();
    void clear();
};

//! \brief A list of \a Section object pointers.
typedef std::vector<Section *> SectionList;

/*! \brief The equivalent of a \a FantomPerformance.
 */
class Track {
public:
    const char *m_name;             //!< Name of this track. Ownership of the string is assumed.
    SectionList m_sectionList;      //!< Section list.
    bool m_chain;                   //!< Chain mode switch. If enabled, FCB1010 program changes are interpreted as 'next' and 'previous' events.
    int m_startSection;             //!< Section index to switch to when this track starts.
    Fantom::Performance *m_performance; //!< Fantom performance for this Track.
    Track(const char *name);
    ~Track();
    void clear();
    //! \brief Add \a Section.
    void addSection(Section *s) { m_sectionList.push_back(s); }
    //! \brief The number of sections in this Track.
    int nofSections() const { return (int)m_sectionList.size(); }
    void merge(Fantom::Performance *performance);
};

typedef std::vector<Track*> TrackList;  //!< A list of \a Track object pointers.

/*! \brief This object contains a list of integers that indicate indexes in
 * a \a TrackList.
 */
class SetList {
    std::vector<int> m_trackIndexList;    //!< List of track indexes.
public:
    //! \brief Return the size ofthe \a SetList.
    int size() const { return (int)m_trackIndexList.size(); }
    //! \brief Return the track index of the i'th item in the \a SetList.
    int operator[](int i) { return i>=0 && i<size() ? m_trackIndexList[i] : 0; }
    //! \brief Append a track index to the \a SetList.
    void add(int i) { m_trackIndexList.push_back(i); }
    //! \brief Append a track by name.
    void add(TrackList &trackList, const char *trackName);
};

void fixChain(TrackList &trackList);
void clear(TrackList &trackList);

#endif // TRACKDEF_H
