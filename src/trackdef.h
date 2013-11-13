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

class FantomPart;
namespace TrackDef {
    static const int Unspecified = -1;
    static const int Previous = -2;
    static const int Current = -3;
    static const int Next = -4;
    static const int Last = -5;
};

class SwPart {
public:
    const char *m_name;
    uint8_t m_channel;
    int m_transpose;
    bool m_customTransposeEnabled;
    int m_customTranspose[12];
    int m_customTransposeOffset;
    uint8_t m_rangeLower;
    uint8_t m_rangeUpper;
    std::vector <const FantomPart *> m_hwPart;
    ControllerRemap *m_controllerRemap; // \TODO: allow more than one
    MonoFilter m_monoFilter;
    Toggler m_toggler;
    bool m_mono;
    Transposer *m_transposer;
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

class Section {
public:
    const char *m_name;
    bool m_noteOffEnter;
    bool m_noteOffLeave;
    std::vector<SwPart *> m_part;
    int m_nextTrack;
    int m_nextSection;
    int m_previousTrack;
    int m_previousSection;
    Section(const char *name, bool noteOffEnter = true, bool noteOffLeave = true);
    ~Section();
    void clear();
    void addPart(SwPart *s) { m_part.push_back(s); }
    int nofParts(void) const { return m_part.size(); }
    void dumpToLog(Screen *screen, const char *prefix) const;
};

class Track {
public:
    const char *m_name;
    std::vector<Section *> m_section;
    bool m_chain;
    int m_startSection;
    Track(const char *name, bool chain = false, int startSection = 0);
    ~Track();
    void clear();
    void addSection(Section *s) { m_section.push_back(s); }
    int nofSections(void) const { return m_section.size(); }
    void dumpToLog(Screen *screen, const char *prefix) const;
};

class SetList {
    std::vector<int> m_list;
public:
    int length(void) const { return m_list.size(); }
    int operator[](int i) { return i>=0 && i<length() ? m_list[i] : 0; }
    void add(int i) { m_list.push_back(i); }
    void add(std::vector<Track*> &tracks, const char *trackName);
};

#endif // TRACKDEF_H
