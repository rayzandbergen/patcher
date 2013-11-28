/*! \file patchercore.cpp
 *  \brief Contains the \a Patcher object and the main entrypoint.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>
#include "trackdef.h"
#include "screen.h"
#include "mididef.h"
#include "mididriver.h"
#include "dump.h"
#include "fantomdriver.h"
#include "transposer.h"
#include "now.h"
#include "timestamp.h"
#include "activity.h"
#include "error.h"
#include "patchercore.h"
#include "networkif.h"
#include "sequencer.h"
#include "timer.h"
#include "xml.h"
#include "persistent.h"
#include "fantomscroller.h"
#include "fcb1010.h"
#include "queue.h"
//#include "undupparts.h"

#define VERSION "1.2.0"     //!< global version number

#define LOG_NOTE            //!< Log note data in MIDI logger window if defined.
#define LOG_CONTROLLER      //!< Log controller data in MIDI logger window if defined.
#define LOG_PROGRAM_CHANGE  //!< Log program change data in MIDI logger window if defined.
#define LOG_PITCHBEND       //!< Log pitch bend data in MIDI logger window if defined.

namespace MetaNote
{
    //! \brief In meta mode, these note numbers are used to select a track.
    enum {
        info = Midi::Note::E4,      //!< Toggle info mode.
        up = Midi::Note::D4,        //!< Move to next track in setlist.
        down = Midi::Note::C4,      //!< Move to previous track in setlist.
        select = Midi::Note::C5     //!< Select directly.
    };
}

//! \brief The master object that handles everything.
class Patcher
{
private:
    const Real debounceTime;            //!< Debounce time in seconds, used to debounce track and section changes.
    ActivityList m_channelActivity;     //!< Per-channel \a Activity.
    ActivityList m_softPartActivity;    //!< Per-\a SwPart \a Activity.
    Screen *m_screen;                   //!< Pointer to global \a Screen object.
    TrackList m_trackList;              //!< Global \a Track list.
    Midi::Driver *m_midi;               //!< Pointer to global MIDI \a Driver object.
    Fantom::Driver *m_fantom;           //!< Pointer to global Fantom \a Driver object.
    //Fantom::Performance *m_performanceList;        //!< Array of \a Performance objects, in the same order as stored in the Fantom.
    int m_trackIdx;                     //!< Current track index
    int m_trackIdxWithinSet;            //!< Current track index within \a SetList.
    SetList m_setList;                  //!< Global \a SetList object.
    int m_sectionIdx;                   //!< Current section index.
    Sequencer m_sequencer;              //!< Global \a Sequencer object.
    Persist m_persist;                  //!< Global \a Persist object.
    bool m_metaMode;                    //!< Meta mode switch.
    FantomScreenScroller m_fantomScroller;              //!< Fantom screen scroller.
    int m_partOffsetBcf;                                //!< Either 0 or 8, since BCF only has 8 sliders to show 16 parameters
    TimeSpec m_debouncePreviousTriggerTime;             //!< Absolute time of last debounce test.
    TimeSpec m_eventRxTime;                             //!< Arrival time of the current Midi event.
    Queue m_eventTxQueue;                      //!< Event queue to write to.
    Track *currentTrack() const {
        return m_trackList[m_trackIdx]; } //!< The current \a Track.
    Section *currentSection() const {
        return currentTrack()->m_sectionList[m_sectionIdx]; } //!< The current \a Section.
    const char *nextTrackName() {
        if (m_trackIdxWithinSet+1 < m_setList.size())
            return m_trackList[m_setList[m_trackIdxWithinSet+1]]->m_name;
        else
            return "<none>";
    } //!< The name of the next \a Track.
    void sendEventToFantom(uint8_t midiStatus,
                uint8_t data1, uint8_t data2 = 255);
    void allNotesOff();
    void changeSection(uint8_t sectionIdx);
    void nextSection();
    void prevSection();
    void changeTrack(uint8_t track, int updateFlags);
    void changeTrackByNote(uint8_t note);
    void updateBcfFaders();
    void updateFantomDisplay();
    void updateScreen();
    bool debounced(Real delaySeconds);
    void consumeSysEx(int device);
public:
    //! \brief Update bit flags.
    enum UpdateFlags {
        UpdateNothing = 0,                 //!< Update nothing.
        UpdateFaders = 1,                  //!< Update BCF faders.
        UpdateFantomDisplay = 2,           //!< Update Fantom display.
        UpdateScreen = 4,                  //!< Update the \a Screen.
        UpdateAll = 7                      //!< Update all.
    };
    size_t nofTracks() const { return m_trackList.size(); } //!< The number of \a Tracks.
    void dumpTrackList();
    void mergePerformanceData();
    void show(int updateFlags);
    void eventLoop();
    void sendReadyEvent();
    void restoreState();
    void loadTrackDefs();
    /*! \brief constructor for Patcher
     *
     *  This will set up an empty Patcher object.
     *
     *  \param [in] s   An initialised \a Screen object.
     *  \param [in] m   An initialised MIDI \a Driver object.
     *  \param [in] f   An initialised Fantom \a Driver object.
    */
    Patcher(Screen *s, Midi::Driver *m, Fantom::Driver *f):
        debounceTime(Real(0.4)),
        m_channelActivity(Midi::NofChannels, Midi::Note::max),
        m_softPartActivity(64 /*see tracks.xsd*/, Midi::Note::max),
        m_screen(s), m_midi(m), m_fantom(f),
        m_trackIdx(0), m_trackIdxWithinSet(0), m_sectionIdx(0),
        m_metaMode(false), m_fantomScroller(f), m_partOffsetBcf(0)
    {
        m_eventTxQueue.openWrite();
        m_trackIdx = m_setList[0];
        getTime(m_debouncePreviousTriggerTime);
        m_fantom->selectPerformanceFromMemCard();
    };
};

/*! \brief Load track definitions from file into the track list and the set list.
 */
void Patcher::loadTrackDefs()
{
    importTracks(TRACK_DEF, m_trackList, m_setList);
}

/*! \brief Dump the track list to a log file, debug only.
 */
void Patcher::dumpTrackList()
{
    FILE *fp = fopen("log.txt", "w");
    if (!fp)
        throw(Error("fopen", errno));
    char prefix[100];
    int i=1;
    for (TrackList::const_iterator t = m_trackList.begin(); t != m_trackList.end(); t++, i++)
    {
        sprintf(prefix, "track%02d", i);
        (*t)->toTextFile(fp, prefix);
    }
    //undupParts(m_performanceList, m_trackList);
}

/*! \brief Add links from software parts to hardware parts, based on matching channels.
 */
void Patcher::mergePerformanceData()
{
    Fantom::PerformanceListLive performanceList;
    const char *cacheFileName = "fantom_cache.bin";
    if (performanceList.readFromCache(cacheFileName) != nofTracks())
    {
        performanceList.download(m_fantom, m_screen->main(), nofTracks());
        performanceList.writeToCache(cacheFileName);
    }
    for (size_t t = 0; t < nofTracks(); t++)
    {
        Track *track = m_trackList[t];
        track->m_performance = performanceList[t];
        for (size_t s = 0; s < track->m_sectionList.size(); s++)
        {
            const Section *section = track->m_sectionList[s];
            for (size_t sp = 0; sp < section->m_partList.size(); sp++)
            {
                SwPart *swPart = section->m_partList[sp];
                for (int hp = 0; hp < Fantom::Performance::NofParts; hp++)
                {
                    const Fantom::Part *hwPart = track->m_performance->m_partList + hp;
                    if (swPart->m_channel == hwPart->m_channel)
                    {
                        swPart->m_hwPartList.push_back(hwPart);
                    }
                }
            }
        }
    }
}

/*! \brief Restore state from a Persist object.
 *
 *  The state consists of the currenct track and current section.
 */
void Patcher::restoreState()
{
    int t,s;
    m_persist.restore(&t, &s);
    changeTrack(t, UpdateAll);
    changeSection(s);
}

/*! \brief Sends a 'ready' event to inform clients of a non MIDI status change.
 */
void Patcher::sendReadyEvent()
{
    Event event;
    event.m_metaMode = m_metaMode ? 1 : 0;
    event.m_currentTrack = m_trackIdx;
    event.m_currentSection = m_sectionIdx;
    event.m_trackIdxWithinSet = m_trackIdxWithinSet;
    event.m_type = Event::Ready;
    event.m_deviceId = Event::Unknown;
    event.m_part = Event::Unknown;
    event.m_midi[0] = Event::Unknown;
    event.m_midi[1] = Event::Unknown;
    event.m_midi[2] = Event::Unknown;
    m_eventTxQueue.send(event);
}

/*! \brief Run the event loop.
 *
 *  This function processes incoming events. It never returns.
 */
void Patcher::eventLoop()
{
    sendReadyEvent();
    for (uint32_t j=0;;j++)
    {
#ifndef RASPBIAN
        if (m_screen->main())
            mvwprintw(m_screen->main(), 1, 69, "%09d\n", j);
#endif
        int deviceRx = m_midi->wait();
        uint8_t byteRx = m_midi->getByte(deviceRx);
        g_timer.setTimeout(false);
        getTime(m_eventRxTime);
        if (m_screen->main())
        {
            m_channelActivity.update(m_eventRxTime);
            m_softPartActivity.update(m_eventRxTime);
        }
        if (byteRx < 0x80)
        {
            // data without status - skip
            int c = byteRx;
            if (!isprint(c))
                c = ' ';
            if (m_screen->log())
                wprintw(m_screen->log(), "dropped %x '%c'\n", byteRx, c);
        }
        else
        {   // start of MIDI message
            switch (byteRx)
            {
                case Midi::timingClock:
                    // timing clock, single byte, dropped
                    break;
                case Midi::activeSensing:
                    // active sensing, single byte, dropped
                    // BUT we abuse the periodic nature of this message
                    // to do a screen update
                    if (m_channelActivity.isDirty())
                        show(UpdateScreen);
                    break;
                case Midi::realtimeStart:
                    if (m_screen->main())
                        mvwprintw(m_screen->main(), 17, 0, "panic on\n");
                    for (int channel=0; channel<Midi::NofChannels; channel++)
                    {
                        m_midi->putBytes(Midi::Device::FantomOut,
                            Midi::controller|channel, Midi::allNotesOff, 0);
                        m_midi->putBytes(Midi::Device::FantomOut,
                            Midi::controller|channel, Midi::resetAllControllers, 0);
                    }
                    break;
                case Midi::realtimeStop:
                    if (m_screen->main())
                        mvwprintw(m_screen->main(), 17, 0, "panic off\n");
                    break;
                case Midi::sysEx:
                    consumeSysEx(deviceRx);
                    break;
                default:
                {
                    // per-channel status switch
                    uint8_t channelRx = byteRx & 0x0f;
                    switch (byteRx & 0xf0)
                    {
                        case Midi::noteOff:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t velo = m_midi->getByte(deviceRx);
#ifdef LOG_NOTE
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "note off %s ch %02x vel %02x\n",
                                    Midi::noteName(note), channelRx, velo);
#endif
                            if (!m_metaMode)
                            {
                                sendEventToFantom(Midi::noteOff, note, velo);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            break;
                        }
                        case Midi::noteOn:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t velo = m_midi->getByte(deviceRx);
#ifdef LOG_NOTE
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "note on %s ch %02x vel %02x\n",
                                    Midi::noteName(note), channelRx, velo);
#endif
                            if (m_metaMode)
                            {
                                if (velo > 0)
                                {
                                    if (note == MetaNote::info)
                                        m_fantomScroller.toggle();
                                    else
                                        changeTrackByNote(note);
                                }
                            }
                            else
                            {
                                sendEventToFantom(Midi::noteOn, note, velo);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            break;
                        }
                        case Midi::aftertouch:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t val = m_midi->getByte(deviceRx);
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "aftertouch %s ch %02x val %02x\n",
                                    Midi::noteName(note), channelRx, val);
                            sendEventToFantom(Midi::aftertouch, note, val);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        case Midi::controller:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
                            uint8_t val = m_midi->getByte(deviceRx);
#ifdef LOG_CONTROLLER
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "controller ch %02x num %02x val %02x\n",
                                    channelRx, num, val);
#endif
                            if ((deviceRx == Midi::Device::A30 || deviceRx == Midi::Device::Fcb1010)
                                && (num == Midi::continuous ||
                                    num == Midi::modulationWheel ||
                                    num == Midi::mainVolume ||
                                    num == Midi::continuousController16 ||
                                    num == Midi::sustain))
                            {
                                sendEventToFantom(Midi::controller, num, val);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            else if (deviceRx == Midi::Device::A30 && num == Midi::effects1Depth)
                            {
                                m_metaMode = !!val;
                                m_midi->putBytes(Midi::Device::BcfOut, Midi::controller|0,
                                    Midi::BCFSwitchA, val ? 127 : 0);
                                show(UpdateScreen);
                                sendReadyEvent();
                            }
                            else if (deviceRx == Midi::Device::BcfIn && num == Midi::BCFSwitchA)
                            {
                                m_metaMode = !!val;
                                show(UpdateScreen);
                                sendReadyEvent();
                            }
                            else if (deviceRx == Midi::Device::BcfIn && num == Midi::effects1Depth)
                            {
                                m_partOffsetBcf ^= 8;
                                show(UpdateFaders);
                            }
                            else if (deviceRx == Midi::Device::BcfIn
                                    && num >= Midi::BCFFader1 && num <= Midi::BCFFader8)
                            {
                                uint8_t partNum = m_partOffsetBcf + (num - Midi::BCFFader1);
                                Fantom::Part *p = currentTrack()->m_performance->m_partList+partNum;
                                p->m_vol = val;
                                m_fantom->setVolume(partNum, val);
                                show(UpdateScreen);
                            }
                            else
                            {
                                if (m_screen->log())
                                    wprintw(m_screen->log(), "dropped controller %02x %02x\n", num, val);
                            }
                            break;
                        }
                        case Midi::programChange:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
#ifdef LOG_PROGRAM_CHANGE
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "program change ch %02x num %02x\n",
                                    channelRx, num);
#endif
                            if (channelRx == masterProgramChangeChannel)
                            {
                                if (num == FCB1010::FootSwitch6)
                                {
                                    m_metaMode = false;
                                    show(UpdateScreen);
                                    sendReadyEvent();
                                }
                                else if (num == FCB1010::FootSwitch7)
                                {
                                    m_metaMode = true;
                                    show(UpdateScreen);
                                    sendReadyEvent();
                                }
                                else if (currentTrack()->m_chain)
                                {
                                    if (num <= FCB1010::FootSwitch2)
                                        prevSection();
                                    else if (num >=FCB1010::FootSwitch3)
                                        nextSection();
                                }
                                else
                                    changeSection(num);
                            }
                            break;
                        }
                        case Midi::channelAftertouch:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "channelRx pressure channelRx %02x num %02x\n",
                                    channelRx, num);
                            sendEventToFantom(Midi::channelAftertouch, num);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        case Midi::pitchBend:
                        {
                            uint8_t num1 = m_midi->getByte(deviceRx);
                            uint8_t num2 = m_midi->getByte(deviceRx);
#ifdef LOG_PITCHBEND
                            if (m_screen->log())
                                wprintw(m_screen->log(),
                                    "pitch bend ch %02x val %04x\n",
                                    channelRx, (uint16_t)num2<<7|(uint16_t)num1);
#endif
                            sendEventToFantom(Midi::pitchBend, num1, num2);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        default:
                            // unknown status, just bail
                            // The next iteration will catch any status-less data.
                            if (m_screen->log())
                                wprintw(m_screen->log(), "unknown %02x\n", byteRx);
                            break;
                    } // END per-channel status switch
                } // END default status byte case
            } // END status byte switch
        } // END of MIDI message
        if (m_screen->log())
            wrefresh(m_screen->log());
        if (m_screen->main())
            wnoutrefresh(m_screen->main());
        if (m_screen->log() || m_screen->main())
            doupdate();
        if (m_metaMode)
            m_fantomScroller.update(m_eventRxTime);
    }
}

/*! \brief Send a MIDI event to the Fantom
 *
 *  This function does all the processing required by the current \a Section
 *  before sending the event off to the Fantom.
 *
 *  \param [in] midiStatus  MIDI status byte
 *  \param [in] data1       MIDI data byte 1
 *  \param [in] data2       MIDI data byte 2, not sent if left to 255
 */
void Patcher::sendEventToFantom(uint8_t midiStatus,
                uint8_t data1, uint8_t data2)
{
    bool drop = false;
    uint8_t data1Out = data1;
    uint8_t data2Out = data2;
    midiStatus &= 0xf0; // just to be sure
    bool isNoteData = Midi::isNote(midiStatus);
    bool isNoteOn = Midi::isNoteOn(midiStatus, data1, data2);
    bool isNoteOff = Midi::isNoteOff(midiStatus, data1, data2);
    bool isController = Midi::isController(midiStatus);
    for (int i=0; i<currentSection()->nofParts(); i++)
    {
        SwPart *swPart = currentSection()->m_partList[i];
        if (isNoteData)
        {
            if (!swPart->inRange(data1))
            {
                continue;
            }
            data1Out = data1 + swPart->m_transpose;
            if (swPart->m_customTransposeEnabled)
                data1Out += swPart->m_customTranspose[
                    (data1 + 12 + swPart->m_customTransposeOffset) % 12];
            if (data1Out > 127)
                data1Out = 127;
            if (swPart->m_mono)
            {
                if (isNoteOn && !swPart->m_monoFilter
                        .passNoteOn(data1, data2))
                {
                    wprintw(m_screen->log(), "note on dropped\n");
                    continue;
                }
                if (isNoteOff && !swPart->m_monoFilter
                        .passNoteOff(data1, data2))
                {
                    wprintw(m_screen->log(), "note off dropped\n");
                    continue;
                }
            }
            if (swPart->m_transposer)
            {
                if (isNoteOn || isNoteOff)
                {
                    swPart->m_transposer->transpose(midiStatus, data1Out, data2Out);
                }
            }
            if (!swPart->m_toggler.pass(midiStatus, data1Out, data2Out))
            {
                wprintw(m_screen->log(), "note on dropped\n");
                continue;
            }
        }
        if (isController)
        {
            if (data1 == Midi::sustain && swPart->m_mono)
            {
                swPart->m_monoFilter.sustain(data2 != 0);
                wprintw(m_screen->log(), "mono sustain\n");
            }
            if (data1 == Midi::sustain && swPart->m_transposer)
            {
                swPart->m_transposer->setSustain(data2 != 0);
                wprintw(m_screen->log(), "transposer sustain\n");
                drop = true;
            }
            swPart->m_controllerRemap->value(data1, data2, &data1Out, &data2Out);
        }
        if (!drop)
        {
            uint8_t messageType;
            if (data2 != 255)
            {
                m_midi->putBytes(Midi::Device::FantomOut,
                    midiStatus|swPart->m_channel, data1Out, data2Out);
                messageType = Event::MidiOut3Bytes;
            }
            else
            {
                m_midi->putBytes(Midi::Device::FantomOut,
                    midiStatus|swPart->m_channel, data1Out);
                messageType = Event::MidiOut2Bytes;
            }
            Event event;
            event.m_metaMode = m_metaMode ? 1 : 0;
            event.m_currentTrack = m_trackIdx;
            event.m_currentSection = m_sectionIdx;
            event.m_trackIdxWithinSet = m_trackIdxWithinSet;
            event.m_type = messageType;
            event.m_deviceId = Midi::Device::FantomOut;
            event.m_part = i;
            event.m_midi[0] = midiStatus|swPart->m_channel;
            event.m_midi[1] = data1Out;
            event.m_midi[2] = data2Out;
            m_eventTxQueue.send(event);
            if (isNoteData)
            {
                if (m_screen->main())
                {
                    m_channelActivity.trigger(swPart->m_channel, data1Out, m_eventRxTime);
                    m_softPartActivity.trigger(i, data1Out, m_eventRxTime);
                }
            }
        }
    } // FOREACH part in current section
}

/*! \brief Update the \a Screen, BCF faders, and the Fantom display.
 * \param [in] updateFlags  Bit field of things to be updated.
 */
void Patcher::show(int updateFlags)
{
    if (updateFlags & UpdateScreen)
        updateScreen();

    if (updateFlags & UpdateFaders)
        updateBcfFaders();

    if (updateFlags & UpdateFantomDisplay)
        updateFantomDisplay();
}

/*! \brief Update the \a Screen.
 *
 * This function updates the main curses() \a Screen.
 */
void Patcher::updateScreen()
{
    if (!m_screen->main())
        return;
    werase(m_screen->main());
    wprintw(m_screen->main(),
        "*** Ray's MIDI patcher " VERSION ", rev " SVN ", " NOW " ***\n\n");
    mvwprintw(m_screen->main(), 2, 0,
        "track   %03d \"%s\"\nsection %03d/%03d \"%s\"\n",
        1+m_trackIdx, currentTrack()->m_name,
        1+m_sectionIdx, currentTrack()->nofSections(),
        currentSection()->m_name);
    mvwprintw(m_screen->main(), 2, 44,
        "%03d/%03d within setlist", 1+m_trackIdxWithinSet, m_setList.size());
    mvwprintw(m_screen->main(), 5, 0,
        "performance \"%s\"\n",
        currentTrack()->m_performance->m_name);
    mvwprintw(m_screen->main(), 3, 44,
        "next \"%s\"\n", nextTrackName());
    mvwprintw(m_screen->main(), 5, 30, "chain mode %s\n",
        currentTrack()->m_chain ?"on":"off");
    if (m_metaMode)
        wattron(m_screen->main(), COLOR_PAIR(1));
    mvwprintw(m_screen->main(), 5, 50, "meta mode %s\n",
        m_metaMode?"on":"off");
    if (m_metaMode)
        wattroff(m_screen->main(), COLOR_PAIR(1));
    int partsShown = 0;
    //ool partActivity[Fantom::Performance::NofParts];
    bool channelActivity[Midi::NofChannels];
    m_channelActivity.get(channelActivity);
    for (int partIdx=0; partIdx<Fantom::Performance::NofParts;partIdx++)
    {
        int x = (partsShown / 4) * 19;
        int y = 7 + partsShown % 4;
        const Fantom::Part *part = currentTrack()->m_performance->m_partList+partIdx;
        ASSERT(part->m_channel != 255);
        if (1 || part->m_channel == m_sectionIdx)
        {
            bool active = channelActivity[part->m_channel];
            if (active)
                wattron(m_screen->main(), COLOR_PAIR(1));
            mvwprintw(m_screen->main(), y, x,
                "%2d %2d %s ", partIdx+1, 1+part->m_channel, part->m_preset);
            if (active)
                wattroff(m_screen->main(), COLOR_PAIR(1));
            partsShown++;
        }
    }
    partsShown = 0;
    const int colLength = 3;
    bool softPartActivity[64];
    m_softPartActivity.get(softPartActivity);
    for (int i=0; i<currentSection()->nofParts(); i++)
    {
        const SwPart *swPart = currentSection()->m_partList[i];
        for (int j=0; j<Fantom::Performance::NofParts;j++)
        {
            const Fantom::Part *hwPart = currentTrack()->m_performance->m_partList+j;
            if (swPart->m_channel == hwPart->m_channel)
            {
                int x = 0;
                if (partsShown >= colLength)
                    x += 40;
                int y = 6 + 5 + 2 + partsShown % colLength;
                if (partsShown % colLength == 0)
                    mvwprintw(m_screen->main(), 6+5+1, x,
                        "prt range         patch        vol tps");
                char keyL[20];
                keyL[0] = 0;
                Midi::noteName(
                    std::max(hwPart->m_keyRangeLower,
                    swPart->m_rangeLower), keyL);
                char keyU[20];
                keyU[0] = 0;
                Midi::noteName(
                    std::min(hwPart->m_keyRangeUpper,
                    swPart->m_rangeUpper), keyU);
                int transpose = swPart->m_transpose +
                        (int)hwPart->m_transpose +
                        (int)hwPart->m_oct*12;
                bool active = softPartActivity[i];
                if (active)
                    wattron(m_screen->main(), COLOR_PAIR(1));
                ASSERT(hwPart->m_patch.m_name[1] != '[');
                mvwprintw(m_screen->main(), y, x,
                    "%3d [%3s - %4s]  %12s %3d %3d", j+1, keyL, keyU,
                    hwPart->m_patch.m_name, hwPart->m_vol,transpose);
                if (active)
                    wattroff(m_screen->main(), COLOR_PAIR(1));
                partsShown++;
            }
        }
    }
}

/*! \brief Abuse the Fantom screen to show the current section.
 */
void Patcher::updateFantomDisplay()
{
    char buf[20];
    sprintf(buf, " [ %d / %d ]",
        1+m_sectionIdx, currentTrack()->nofSections());
    m_fantom->setPartName(0, buf);
}

/*! \brief Update the BCF faders.
 *
 * This function slides the motorised BCF faders into positions that
 * show the current hardware part volumes.
 * The octave shift is set as well, at the turning knobs. The BCF
 * can only show 8 parts at a time, so one of the BCF's buttons is
 * used to switch between part 1-8 and 9-16.
 */
void Patcher::updateBcfFaders()
{
    for (int i=0; i<Fantom::Performance::NofParts;i++)
    {
        const Fantom::Part *hwPart = currentTrack()->m_performance->m_partList+i;
        if (i >= m_partOffsetBcf && i< m_partOffsetBcf + 8)
        {
            int showPart = i - m_partOffsetBcf;
            m_midi->putBytes(Midi::Device::BcfOut, Midi::controller|0,
                Midi::BCFSpinner1+showPart, 0x80 + 4 * hwPart->m_oct);
            m_midi->putBytes(Midi::Device::BcfOut, Midi::controller|0,
                Midi::BCFFader1+showPart, hwPart->m_vol);
        }
    }
}

/*! \brief Send all notes off on all channels in use by the current \a Section.
 *
 * Sustain controller is reset as well.
 */
void Patcher::allNotesOff()
{
    bool channelsCleared[Midi::NofChannels];
    for (int i=0; i<Midi::NofChannels; i++)
        channelsCleared[i] = false;
    for (int i=0; i<currentSection()->nofParts(); i++)
    {
        SwPart *part = currentSection()->m_partList[i];
        int channel = part->m_channel;
        if (!channelsCleared[i])
        {
            channelsCleared[i] = true;
            m_midi->putBytes(
                Midi::Device::FantomOut,
                Midi::controller|channel, Midi::allNotesOff, 0);
            m_midi->putBytes(
                Midi::Device::FantomOut,
                Midi::controller|channel, Midi::sustain, 0);
            if (m_screen->log())
                wprintw(m_screen->log(), "all notes off ch %02x\n", channel+1);
            part->m_toggler.reset();
        }
    }
}

/*! \brief Change to a new \a Section.
 *
 * This function will also handle 'all notes off' and screen / BCF updates.
 *
 * \param [in] sectionIdx   The section number to switch to.
 */
void Patcher::changeSection(uint8_t sectionIdx)
{
    if (sectionIdx <
        m_trackList[m_trackIdx]->nofSections())

    {
        if (currentSection()->m_noteOffLeave ||
                currentTrack()->m_sectionList[sectionIdx]->m_noteOffEnter)
        {
            allNotesOff();
        }
        m_sectionIdx = sectionIdx;
        show(UpdateScreen|UpdateFantomDisplay);
        m_persist.store(m_trackIdx, m_sectionIdx);
        sendReadyEvent();
    }
}

/*! \brief Check if debounce delay has expired.
 *
 * This function returns true if the previous call to this function is
 * more than delaySeconds ago.
 *
 * \param [in] delaySeconds  The minimum delay in seconds.
 */
bool Patcher::debounced(Real delaySeconds)
{
    bool rv = timeDiffSeconds(m_debouncePreviousTriggerTime, m_eventRxTime) > delaySeconds;
    if (rv)
    {
        m_debouncePreviousTriggerTime = m_eventRxTime;
    }
    return rv;
}

/*! \brief Change to next \a Section.
 *
 * This function switches to the next \a Section and possibly even to the
 * next \a Track, according this \a Section's chaining settings.
 * This also handles debouncing, since the FCB has a tendency to send
 * multiple events on a single pedal press.
 *
 */
void Patcher::nextSection()
{
    if (!debounced(debounceTime))
        return;
    int newTrack = currentSection()->m_nextTrack;
    int newSection = currentSection()->m_nextSection;
    if (newTrack != m_trackIdx)
    {
        changeTrack(newTrack, UpdateAll);
        changeSection(newSection);
    }
    else if (newSection != m_sectionIdx)
    {
        changeSection(newSection);
    }
}

/*! \brief Change to previous \a Section.
 *
 * \sa Patcher::nextSection().
 *
 */
void Patcher::prevSection()
{
    if (!debounced(debounceTime))
        return;
    int newTrack = currentSection()->m_previousTrack;
    int newSection = currentSection()->m_previousSection;
    if (newTrack != m_trackIdx)
    {
        changeTrack(newTrack, UpdateAll);
        changeSection(newSection);
    }
    else if (newSection != m_sectionIdx)
    {
        changeSection(newSection);
    }
}

/*! \brief Switch to a new \a Track.
 */
void Patcher::changeTrack(uint8_t track, int updateFlags)
{
    //wprintw(m_screen->log(), "change track %d\n", track);
    m_trackIdx = track;
    m_sectionIdx = currentTrack()->m_startSection; // cannot use changeSection!
    m_fantom->selectPerformance(m_trackIdx);
    show(updateFlags);
    m_persist.store(m_trackIdx, m_sectionIdx);
}

/*! \brief Switch to \a Track indicated by note number.
 *
 * \sa MetaNote::select.
 *
 * param [in] note  MIDI note number.
 */
void Patcher::changeTrackByNote(uint8_t note)
{
    bool valid = false;
    if (note == MetaNote::up && m_trackIdxWithinSet < m_setList.size()-1)
    {
        m_trackIdxWithinSet++;
        m_trackIdx = m_setList[m_trackIdxWithinSet];
        valid = true;
    }
    else if (note == MetaNote::down && m_trackIdxWithinSet > 0)
    {
        m_trackIdxWithinSet--;
        m_trackIdx = m_setList[m_trackIdxWithinSet];
        valid = true;
    }
    else if (note >= MetaNote::select
            && note < MetaNote::select + nofTracks())
    {
        m_trackIdx = note - MetaNote::select;
        valid = true;
    }
    if (valid)
    {
        changeTrack((uint8_t)m_trackIdx, UpdateAll);
    }
    sendReadyEvent();
}

/*! \brief Consume a sysEx message from a device.
 *
 * This function will consume bytes until an EOX is seen.
 * \param [in] device   The MIDI device number.
 */
void Patcher::consumeSysEx(int device)
{
    if (m_screen->log())
        wprintw(m_screen->log(), "sysEx ");
    for (;;)
    {
        uint8_t byteRx = m_midi->getByte(device);
        int c = byteRx;
        if (!isprint(c))
            c = ' ';
        if (m_screen->log())
            wprintw(m_screen->log(), "%x '%c' ", byteRx, c);
        if (byteRx == Midi::EOX)
            break;
    }
    if (m_screen->log())
        wprintw(m_screen->log(), "\n");
}

/*! \brief The main entrypoint of the patcher.
 *
 * There is only one optional command line argument: The directory to which to change.
 */

int main(int argc, char **argv)
{
    try
    {
        bool mainWindow = false;
        bool logWindow = false;
        for (;;)
        {
            int opt = getopt(argc, argv, "lmhd:");
            if (opt == -1)
                break;
            switch (opt)
            {
                case 'l':
                    logWindow = true;
                    break;
                case 'm':
                    mainWindow = true;
                    break;
                case 'd':
                {
                    const char *dir = optarg;
                    if (-1 == chdir(dir))
                    {
                        Error e;
                        e.stream() << "cannot change dir to " << dir;
                        throw(e);
                    }
                    break;
                }
                default:
                    std::cerr << "\npatcher [-h|?] [-d <dir>] [-m] [-l]\n\n"
                        "  -h|?     This message\n"
                        "  -d dir   Change dir\n"
                        "  -m       Show main window\n"
                        "  -l       Show log window\n\n";
                    return 0;
                    break;
            }
        }
        if (argc > optind)
        {
            throw(Error("unrecognised trailing arguments, try -h"));
        }
#if 0
        // debug: see if we have memory leaks if we read
        // the config file and then clean up again.

        TrackList trackList;
        SetList setList;
        importTracks(TRACK_DEF, trackList, setList);
        clear(trackList);
        return 0;
#endif
        g_timer.setTimeout((Real)2.5);
        Screen screen(mainWindow, logWindow);
        if (mainWindow)
            wprintw(screen.main(), "   *** initialising ***\n\n");
        Midi::Driver midi(screen.log());
        Fantom::Driver fantom(screen.log(), &midi);
        Patcher patcher(&screen, &midi, &fantom);
        patcher.loadTrackDefs();
        patcher.mergePerformanceData();
        //patcher.dumpTrackList();
        patcher.restoreState();
        patcher.show(Patcher::UpdateScreen|Patcher::UpdateFaders);
        patcher.eventLoop();
    }
    catch (Error &e)
    {
        endwin();
        std::cerr << "** " << e.what() << std::endl;
        return e.exitCode();
    }
    catch (...)
    {
        endwin();
        std::cerr << "unhandled exception" << std::endl;
    }
    return -127;
}
