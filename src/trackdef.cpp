/*! \file trackdef.cpp
 *  \brief Contains objects to handle track definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "trackdef.h"
#include "fantomdriver.h"
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
    for (SwPartList::iterator i = m_partList.begin(); i != m_partList.end(); ++i)
        delete *i;
    m_partList.clear();
    free((void*)m_name);
    m_name = 0;
}

//! \brief Contruct a default SwPart with a given name.
SwPart::SwPart(int number, const char *name):
        m_number(number),
        m_name(name), m_channel(255), m_transpose(0),
        m_customTransposeEnabled(false),
        m_rangeLower(0), m_rangeUpper(127),
        m_controllerRemap(0),
        m_mono(false),
        m_transposer(0)
{
    memset(&m_customTranspose, 0, sizeof m_customTranspose);
}

//! \brief Destructor.
SwPart::~SwPart()
{
    clear();
}

//! \brief Clean up memory used by this \a SwPart..
void SwPart::clear()
{
    if (m_controllerRemap)
        delete m_controllerRemap;
    m_controllerRemap = 0;
    delete m_transposer;
    m_transposer = 0;
    free((void*)m_name);
    m_name = 0;
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

/*! \brief Merge performance data.
 */
void Track::merge(Fantom::Performance *performance)
{
    m_performance = performance;
    // FOREACH section
    for (SectionList::const_iterator section = m_sectionList.begin();
            section != m_sectionList.end(); ++section)
    {
        // FOREACH software part
        for (SwPartList::const_iterator sp = (*section)->m_partList.begin();
                sp != (*section)->m_partList.end(); ++sp)
        {
            SwPart *swPart = (*sp);
            // FOREACH hardware part
            for (int hp = 0; hp < Fantom::Performance::NofParts; hp++)
            {
                Fantom::Part *hwPart = m_performance->m_partList + hp;
                // create double link if channels match
                if (swPart->m_channel == hwPart->m_channel)
                {
                    swPart->m_hwPartList.push_back(hwPart);
                    hwPart->m_swPartList.push_back(swPart);
                }
            }
        }
    }
}

//! \brief Clean up memory used by this \a Track.
void Track::clear()
{
    for (SectionList::iterator i = m_sectionList.begin(); i != m_sectionList.end(); ++i)
        delete *i;
    m_sectionList.clear();
    free((void*)m_name);
    m_name = 0;
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
    for (TrackList::iterator i = trackList.begin(); i != trackList.end(); ++i)
        delete *i;
}
