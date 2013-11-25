/*! \file trackdef.cpp
 *  \brief Contains objects to handle track definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "trackdef.h"
#include "fantom.h"
#include "error.h"

//! \brief Construct a default Section with a given name.
Section::Section(const char *name):
        m_name(name), m_noteOffEnter(true), m_noteOffLeave(true),
        m_nextTrack(TrackDef::Unspecified),
        m_nextSection(TrackDef::Unspecified),
        m_previousTrack(TrackDef::Unspecified),
        m_previousSection(TrackDef::Unspecified)
{
}

//! \brief Destructor.
Section::~Section()
{
    clear();
}

//! \brief Clean up memory used by this \a Section.
void Section::clear()
{
    for (SwPartList::iterator i = m_partList.begin(); i != m_partList.end(); i++)
        delete *i;
    m_partList.clear();
    free((void*)m_name);
    m_name = 0;
}

//! \brief Contruct a default SwPart with a given name.
SwPart::SwPart(const char *name):
        m_name(name), m_channel(255), m_transpose(0),
        m_customTransposeEnabled(false),
        m_rangeLower(0), m_rangeUpper(127),
        m_controllerRemap(new ControllerRemap::Default),
        m_mono(false),
        m_transposer(0)
{
}

//! \brief Destructor.
SwPart::~SwPart()
{
    clear();
}

//! \brief Clean up memory used by this \a SwPart..
void SwPart::clear()
{
    delete m_controllerRemap;
    m_controllerRemap = 0;
    delete m_transposer;
    m_transposer = 0;
    free((void*)m_name);
    m_name = 0;
}

//! \brief Convert a string to a MIDI note number.
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
    ASSERT(num != 255);
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

//! \brief Construct a default Track with a given name.
Track::Track(const char *name): m_name(name), m_chain(false), m_startSection(0), m_performance(0)
{
}

//! \brief Destructor.
Track::~Track()
{
    clear();
}

//! \brief Clean up memory used by this \a Track.
void Track::clear()
{
    for (SectionList::iterator i = m_sectionList.begin(); i != m_sectionList.end(); i++)
        delete *i;
    m_sectionList.clear();
    free((void*)m_name);
    m_name = 0;
}

//! \brief Dump to a log file.
void Track::toTextFile(FILE *fp, const char *prefix) const
{
    fprintf(fp, "%s.name:%s\n", prefix, m_name);
    fprintf(fp, "%s.chain:%d\n", prefix, m_chain);
    fprintf(fp, "%s.startSection:%d\n", prefix, m_startSection);
    fprintf(fp, "%s.nofSections:%d\n", prefix, nofSections());
    char nestedPrefix[100];
    for (int i=0; i<nofSections(); i++)
    {
        sprintf(nestedPrefix, "%s.section%02d", prefix, 1+i);
        m_sectionList[i]->toTextFile(fp, nestedPrefix);
    }
}

//! \brief Dump to a log file.
void Section::toTextFile(FILE *fp, const char *prefix) const
{
    fprintf(fp, "%s.name:%s\n", prefix, m_name);
    fprintf(fp, "%s.noteOffEnter:%d\n", prefix, m_noteOffEnter);
    fprintf(fp, "%s.noteOffLeave:%d\n", prefix, m_noteOffLeave);
    fprintf(fp, "%s.nextTrack:%d\n", prefix, m_nextTrack);
    fprintf(fp, "%s.nextSection:%d\n", prefix, m_nextSection);
    fprintf(fp, "%s.previousTrack:%d\n", prefix, m_previousTrack);
    fprintf(fp, "%s.previousSection:%d\n", prefix, m_previousSection);
    fprintf(fp, "%s.nofSwParts:%d\n", prefix, nofParts());
    char nestedPrefix[100];
    for (int i=0; i<nofParts(); i++)
    {
        sprintf(nestedPrefix, "%s.swPart%02d", prefix, 1+i);
        m_partList[i]->toTextFile(fp, nestedPrefix);
    }
}

//! \brief Dump to a log file.
void SwPart::toTextFile(FILE *fp, const char *prefix) const
{
    fprintf(fp, "%s.name:%s\n", prefix, m_name ? m_name : "noName");
    fprintf(fp, "%s.channel:%d\n", prefix, m_channel);
    fprintf(fp, "%s.transpose:%d\n", prefix, m_transpose);
    fprintf(fp, "%s.rangeLower:%s\n", prefix, Midi::noteName(m_rangeLower));
    fprintf(fp, "%s.rangeUpper:%s\n", prefix, Midi::noteName(m_rangeUpper));
    fprintf(fp, "%s.nofHwParts:%d\n", prefix, (int)m_hwPartList.size());
    char nestedPrefix[100];
    for (size_t i=0; i<m_hwPartList.size(); i++)
    {
        sprintf(nestedPrefix, "%s.hwPart%02d", prefix, 1+(int)m_hwPartList[i]->m_number);
        m_hwPartList[i]->toTextFile(fp, nestedPrefix);
    }
}

void SetList::add(TrackList &trackList, const char *trackName)
{
    int addIdx = TrackDef::Unspecified;
    for (int i=0; i<(int)trackList.size(); i++)
    {
        if (strncasecmp(trackName, trackList[i]->m_name, strlen(trackName)) == 0)
        {
            addIdx = i;
            break;
        }
    }
    if (addIdx == TrackDef::Unspecified)
    {
        Error e;
        e.stream() << "SetList::add() cannot find track " << trackName << " by name";
        throw(e);
    }
    add(addIdx);
}

//! \brief Fix chain info in a track list
void fixChain(TrackList &trackList)
{
    for (size_t t=0; t<trackList.size(); t++)
    {
        Track *track = trackList[t];
        size_t tPrev = t > 0 ? t-1 : t; // clamped
        size_t tNext = t+1 < trackList.size() ? t+1 : t; // clamped
        for (size_t s=0; s<track->m_sectionList.size(); s++)
        {
            Section *section = track->m_sectionList[s];
            size_t sNext = s+1 < track->m_sectionList.size() ? s+1 : 0; // wraparound
            size_t sPrev = s > 0 ? s-1 : track->m_sectionList.size()-1; // wraparound
            switch (section->m_nextTrack)
            {
                case TrackDef::NextTrack:
                    section->m_nextTrack = tNext;
                    break;
                case TrackDef::Unspecified:
                case TrackDef::CurrentTrack:
                    section->m_nextTrack = t;
                    break;
                case TrackDef::PreviousTrack:
                    section->m_nextTrack = tPrev;
                    break;
                case TrackDef::LastSection:
                    INTERNAL_ERROR;
                    break;
                default:
                    if (section->m_nextTrack >= (int)trackList.size() || section->m_nextTrack < 0)
                        section->m_nextTrack = t;
            }
            switch (section->m_previousTrack)
            {
                case TrackDef::NextTrack:
                    section->m_previousTrack = tNext;
                    break;
                case TrackDef::Unspecified:
                case TrackDef::CurrentTrack:
                    section->m_previousTrack = t;
                    break;
                case TrackDef::PreviousTrack:
                    section->m_previousTrack = tPrev;
                    break;
                case TrackDef::LastSection:
                    INTERNAL_ERROR;
                    break;
                default:
                    if (section->m_previousTrack >= (int)trackList.size() || section->m_previousTrack < 0)
                        section->m_previousTrack = t;
            }
            size_t sLast = trackList[section->m_nextTrack]->m_sectionList.size()-1;
            switch (section->m_nextSection)
            {
                case TrackDef::Unspecified:
                    section->m_nextSection = sNext;
                    break;
                case TrackDef::LastSection:
                    section->m_nextSection = sLast;
                    break;
                case TrackDef::PreviousTrack:
                case TrackDef::NextTrack:
                case TrackDef::CurrentTrack:
                    INTERNAL_ERROR;
                    break;
                default:
                    if (section->m_nextSection > (int)sLast || section->m_nextSection < 0)
                        section->m_nextSection = (int)(s <= sLast ? s : sLast);
            }
            sLast = trackList[section->m_previousTrack]->m_sectionList.size()-1;
            switch (section->m_previousSection)
            {
                case TrackDef::Unspecified:
                    section->m_previousSection = sPrev;
                    break;
                case TrackDef::LastSection:
                    section->m_previousSection = sLast;
                    break;
                case TrackDef::PreviousTrack:
                case TrackDef::NextTrack:
                case TrackDef::CurrentTrack:
                    INTERNAL_ERROR;
                    break;
                default:
                    if (section->m_previousSection > (int)sLast || section->m_previousSection < 0)
                        section->m_previousSection = (int)(s <= sLast ? s : sLast);
            }
        } // FOREACH section
    } // FOREACH track
}

/*! \brief Clean up a track list.
 *
 * Not really needed, only for memory leak checks.
 */
void clear(TrackList &trackList)
{
    for (TrackList::iterator i = trackList.begin(); i != trackList.end(); i++)
        delete *i;
}
