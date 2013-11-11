#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "trackdef.h"
#include "fantom.h"
#include "error.h"

Section::Section(const char *name, bool noteOffEnter, bool noteOffLeave):
        m_name(name), m_noteOffEnter(noteOffEnter), m_noteOffLeave(noteOffLeave),
        m_nextTrack(TrackDef::Unspecified),
        m_nextSection(TrackDef::Unspecified),
        m_previousTrack(TrackDef::Unspecified),
        m_previousSection(TrackDef::Unspecified)
{
}

Section::~Section()
{
    clear();
}

void Section::clear()
{
    for (std::vector<SwPart *>::iterator i = m_part.begin(); i != m_part.end(); i++)
        delete *i;
    m_part.clear();
    free((void*)m_name);
    m_name = 0;
}

SwPart::SwPart(const char *name, uint8_t channel, int transpose,
            const char *lower, const char *upper,
            bool mono, Transposer *t):
        m_name(name), m_channel(channel), m_transpose(transpose),
        m_customTransposeEnabled(false), m_mono(mono), m_transposer(t)
{
    m_rangeLower = lower ? stringToNoteNum(lower) : 0;
    m_rangeUpper = upper ? stringToNoteNum(upper): 127;
    m_controllerRemap = new ControllerRemap;
}

SwPart::~SwPart()
{
    clear();
}

void SwPart::clear()
{
    delete m_controllerRemap;
    m_controllerRemap = 0;
    delete m_transposer;
    m_transposer = 0;
    free((void*)m_name);
    m_name = 0;
}

uint8_t SwPart::stringToNoteNum(const char *s)
{
    const struct NoteOffset {
        char m_char;
        uint8_t m_offset;
    } noteOffset[] = {
        {'C', 0 },
        {'D', 2 },
        {'E', 4 },
        {'F', 5 },
        {'G', 7 },
        {'A', 9 },
        {'B', 11 }
    };

    uint8_t num = 255;
    int c = toupper(s[0]);
    for (uint8_t i=0; i<7; i++)
        if (c == noteOffset[i].m_char)
        {
            num = noteOffset[i].m_offset;
            break;
        }
    if (num == 255)
        throw(Error("SwPart::stringToNoteNum internal error"));
    uint8_t i = 1;
    if (s[i] == '#')
    {
        num++;
        i++;
    }
    else if (s[i] == 'b')
    {
        num--;
        i++;
    }
    uint8_t oct = s[i++] - '0';
    if (s[i])
    {
        oct = 10*oct + (s[i] - '0');
    }
    num += (uint8_t)12 * oct;
    return num;
}

Track::Track(const char *name, bool chain, int startSection): m_name(name), m_chain(chain), m_startSection(startSection)
{
}

Track::~Track()
{
    clear();
}

void Track::clear()
{
    for (std::vector<Section *>::iterator i = m_section.begin(); i != m_section.end(); i++)
        delete *i;
    m_section.clear();
    free((void*)m_name);
    m_name = 0;
}

void Track::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name);
    screen->printLog("%s.chain:%d\n", prefix, m_chain);
    screen->printLog("%s.startSection:%d\n", prefix, m_startSection);
    screen->printLog("%s.nofSections:%d\n", prefix, nofSections());
    char nestedPrefix[100];
    for (int i=0; i<nofSections(); i++)
    {
        sprintf(nestedPrefix, "%s.section%02d", prefix, 1+i);
        m_section[i]->dumpToLog(screen, nestedPrefix);
    }
}

void Section::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name);
    screen->printLog("%s.noteOffEnter:%d\n", prefix, m_noteOffEnter);
    screen->printLog("%s.noteOffLeave:%d\n", prefix, m_noteOffLeave);
    screen->printLog("%s.nextTrack:%d\n", prefix, m_nextTrack);
    screen->printLog("%s.nextSection:%d\n", prefix, m_nextSection);
    screen->printLog("%s.previousTrack:%d\n", prefix, m_previousTrack);
    screen->printLog("%s.previousSection:%d\n", prefix, m_previousSection);
    screen->printLog("%s.nofSwParts:%d\n", prefix, nofParts());
    char nestedPrefix[100];
    for (int i=0; i<nofParts(); i++)
    {
        sprintf(nestedPrefix, "%s.swPart%02d", prefix, 1+i);
        m_part[i]->dumpToLog(screen, nestedPrefix);
    }
}

void SwPart::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name ? m_name : "noName");
    screen->printLog("%s.channel:%d\n", prefix, m_channel);
    screen->printLog("%s.transpose:%d\n", prefix, m_transpose);
    screen->printLog("%s.rangeLower:%s\n", prefix, Midi::noteName(m_rangeLower));
    screen->printLog("%s.rangeUpper:%s\n", prefix, Midi::noteName(m_rangeUpper));
    screen->printLog("%s.nofHwParts:%d\n", prefix, m_hwPart.size());
    char nestedPrefix[100];
    for (size_t i=0; i<m_hwPart.size(); i++)
    {
        sprintf(nestedPrefix, "%s.hwPart%02d", prefix, 1+(int)m_hwPart[i]->m_number);
        m_hwPart[i]->dumpToLog(screen, nestedPrefix);
    }
}

void SetList::add(std::vector<Track*> &tracks, const char *trackName)
{
    int addIdx = TrackDef::Unspecified;
    for (int i=0; i<(int)tracks.size(); i++)
    {
        if (strncasecmp(trackName, tracks[i]->m_name, strlen(trackName)) == 0)
        {
            addIdx = i;
            break;
        }
    }
    if (addIdx == TrackDef::Unspecified)
    {
        printf("cannot find track %s by name\n", trackName);
        exit(1);
    }
    add(addIdx);
    //printf("added track %d to setlist: %s\n", addIdx, trackName);
}

void initSetList(std::vector<Track*> &tracks, SetList &setList)
{
    // set 1
    setList.add(tracks, "Overture 1928 2");
    setList.add(tracks, "Overture 1928");
    setList.add(tracks, "Lines in the sand 1/2");
    setList.add(tracks, "Lines in the sand 2/2");
    setList.add(tracks, "Another day");
    setList.add(tracks, "Scarred");
    setList.add(tracks, "The Mirror");

    // set 2
    setList.add(tracks, "About to crash");
    setList.add(tracks, "Pull me under");
    setList.add(tracks, "Erotomania");
    setList.add(tracks, "Overture 1928"); // voices

    // set 3
    setList.add(tracks, "Home");
    setList.add(tracks, "As i am");
    setList.add(tracks, "Metropolis part I");
    setList.add(tracks, "Peruvian skies");
    setList.add(tracks, "Learning to live");

    // encore
    setList.add(tracks, "Under a glass moon");
    setList.add(tracks, "The count of Tuscany");
#if 0
    // set 1
    setList.add(tracks, "Overture 1928 2");
    setList.add(tracks, "Overture 1928");
    setList.add(tracks, "Erotomania");
    setList.add(tracks, "Overture 1928"); // voices
    setList.add(tracks, "Under a glass moon");
    setList.add(tracks, "Another day");
    setList.add(tracks, "Metropolis part I");
    setList.add(tracks, "Trial of tears");
    setList.add(tracks, "Only a matter of time");
    // set 2
    setList.add(tracks, "Home");
    setList.add(tracks, "Pull me under");
    setList.add(tracks, "Take the time");
    setList.add(tracks, "As i am");
    setList.add(tracks, "Lines in the sand 1/2");
    setList.add(tracks, "Lines in the sand 2/2");
    setList.add(tracks, "Peruvian skies");
    setList.add(tracks, "Learning to live");
    // encore
    setList.add(tracks, "The count of Tuscany");
    //for (int i=0; i<tracks.size(); i++) tracks[i]->dumpToLog();
#endif
    //exit(1);
}
