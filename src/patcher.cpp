#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <ctype.h>
#include <assert.h>

#include "trackdef.h"
#include "screen.h"
#include "midi.h"
#include "dump.h"
#include "fantom.h"
#include "transposer.h"
#include "now.h"
#include "timestamp.h"
#include "activity.h"
#include "error.h"
#include "patcher.h"
#include "midi_note.h"
#include "networkif.h"
#include "sequencer.h"
#include "alarm.h"
#include "xml.h"
#include "persistent.h"

#define VERSION "1.1.0"

// events shown in the midi logger window
#define LOG_NOTE
#define LOG_CONTROLLER
#define LOG_PROGRAM_CHANGE
#define LOG_PITCHBEND

// in meta mode note numbers are used to select a track
namespace MetaNote
{
    enum {
        info = MidiNote::E4,      //      toggle info mode
        up = MidiNote::D4,        //      move to next track in setlist
        down = MidiNote::C4,      //      move to previous track in setlist
        select = MidiNote::C5     //      select directly
    };
};

class Patcher
{
    const Real debounceTime;
    Activity m_channelActivity;
    Activity m_softPartActivity;
    Screen *m_screen;
    std::vector <Track *>m_trackList;
    Midi *m_midi;
    Fantom *m_fantom;
    FantomPerformance *m_perf;
    int m_trackIdx;
    int m_trackIdxWithinSet;
    SetList m_setList;
    int m_sectionIdx;
    Sequencer m_sequencer;
    Persist m_persist;
    Track *currentTrack(void) const {
        return m_trackList[m_trackIdx]; }
    FantomPerformance *currentPerf(void) const {
        return &m_perf[m_trackIdx]; }
    Section *currentSection(void) const {
        return currentTrack()->m_section[m_sectionIdx]; }
    int nofTracks(void) const { return m_trackList.size(); }
    const char *nextTrackName(void) {
        if (m_trackIdxWithinSet+1 < m_setList.length())
            return m_trackList[m_setList[m_trackIdxWithinSet+1]]->m_name;
        else
            return "<none>";
    }
    int m_metaMode;
    struct {
        int m_mode;
        struct timespec m_previous;
        size_t m_offset;
        std::string m_text;
    } m_info;
    int m_partOffsetBcf;
    struct timespec m_debouncePrev;
    void sendEventToFantom(uint8_t midiStatus,
                uint8_t data1, uint8_t data2 = 255);
    void allNotesOff(void);
    void changeSection(uint8_t sectionIdx);
    void nextSection(void);
    void prevSection(void);
    void changeTrack(uint8_t track, int updateFlags);
    void changeTrackByNote(uint8_t note);
    void toggleInfoMode(uint8_t note);
    void showInfo();
    void updateBcfFaders(void);
    bool debounced(Real delaySeconds);
    void consumeSysEx(int device);
public:
    static const int UpdateNothing = 0;
    static const int UpdateFaders = 1;
    static const int UpdateFantomDisplay = 2;
    static const int UpdateScreen = 4;
    static const int UpdateAll = 7;
    void dumpTrackList();
    void downloadPerfomanceData(void);
    void mergePerformanceData();
    void show(int updateFlags);
    void eventLoop(void);
    void loadTrackDefs(void);
    Patcher(Screen *s, Midi *m, Fantom *f):
        debounceTime(Real(0.4)),
        m_screen(s), m_midi(m), m_fantom(f),
        m_trackIdx(0), m_trackIdxWithinSet(0), m_sectionIdx(0),
        m_metaMode(0), m_partOffsetBcf(0)
    {
        m_info.m_mode = 0;
        m_info.m_offset = 0;
        m_trackIdx = m_setList[0];
        getTime(&m_debouncePrev);
        m_info.m_previous = m_debouncePrev;
    };
};

void Patcher::loadTrackDefs(void)
{
    importTracks(TRACK_DEF, m_trackList);
    initSetList(m_trackList, m_setList);
}

void Patcher::dumpTrackList()
{
    char prefix[100];
    for (int i=0; i<nofTracks(); i++)
    {
        sprintf(prefix, "track%02d", 1+i);
        m_trackList[i]->dumpToLog(m_screen, prefix);
    }
}

void Patcher::downloadPerfomanceData(void)
{
    const char *fantomPatchFile = "fantom_cache.bin";

    m_perf = new FantomPerformance[nofTracks()];
    // bank select 'Card, performance'
    // This only works when Fantom is already in performance mode
    m_midi->putBytes(MidiDevice::FantomOut,
        MidiStatus::controller|Fantom::programChangeChannel, 0x00, 85);
    m_midi->putBytes(MidiDevice::FantomOut,
        MidiStatus::controller|Fantom::programChangeChannel, 0x20, 32);
    m_midi->putBytes(MidiDevice::FantomOut,
        MidiStatus::programChange|Fantom::programChangeChannel, 0);
    Dump d;
    if (d.fopen(fantomPatchFile, O_RDONLY, 0))
    {
        for (int i=0; i<nofTracks(); i++)
        {
            m_perf[i].restore(&d);
        }
        d.fclose();
        return;
    }
    char s[20];
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10*1000*1000;
    for (int i=0; i<nofTracks(); i++)
    {
        changeTrack(i, UpdateNothing);
        nanosleep(&ts, NULL);
        m_fantom->getPerfName(s);
        g_alarm.m_doTimeout = false;
        m_screen->printMidi("%3.0f%% reading '%s'\n", 100.0*(i+1.0)/nofTracks(), s);
        m_screen->flushMidi();
        strcpy(m_perf[i].m_name, s);
        for (int j=0; j<16; j++)
        {
            FantomPart *hwPart = m_perf[i].m_part+j;
            //nanosleep(&ts, NULL);
            m_fantom->getPartParams(hwPart, j);
            bool readPatchParams;
            hwPart->m_number = j;
            hwPart->constructPreset(readPatchParams);
            if (readPatchParams)
            {
                m_fantom->getPatchName(hwPart->m_patch.m_name, j);
            }
            else
            {
                strcpy(hwPart->m_patch.m_name, "secret GM   ");
            }
            m_screen->flushMidi();
        }
    }
    if (!d.fopen(fantomPatchFile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR))
    {
        throw(Error("open O_WRONLY|O_CREAT", errno));
    }
    else
    {
        for (int i=0; i<nofTracks(); i++)
        {
            m_perf[i].save(&d);
        }
        d.fclose();
    }
}

void Patcher::mergePerformanceData()
{
    // add links from swParts to hwParts, based on matching channels
    for (int t = 0; t < nofTracks(); t++)
    {
        const Track *track = m_trackList[t];
        const FantomPerformance *perf = m_perf+t;
        for (size_t s = 0; s < track->m_section.size(); s++)
        {
            const Section *section = track->m_section[s];
            for (size_t sp = 0; sp < section->m_part.size(); sp++)
            {
                SwPart *swPart = section->m_part[sp];
                for (int hp = 0; hp < 16; hp++)
                {
                    const FantomPart *hwPart = perf->m_part + hp;
                    if (swPart->m_channel == hwPart->m_channel)
                    {
                        swPart->m_hwPart.push_back(hwPart);
                    }
                }
            }
        }
    }
}

void Patcher::eventLoop(void)
{
    //changeTrack(m_setList[0]);
    {
        int t,s;
        m_persist.restore(&t, &s);
        changeTrack(t, UpdateAll);
        changeSection(s);
    }
    for (uint32_t j=0;;j++)
    {
#ifndef RASPBIAN
        mvwprintw(m_screen->m_track, 1, 69, "%09d\n", j);
#endif
        m_channelActivity.clean();
        m_softPartActivity.clean();
        int deviceRx = m_midi->wait();
        uint8_t byteRx = m_midi->getByte(deviceRx);
        g_alarm.m_doTimeout = false;
        if (byteRx < 0x80)
        {
            // data without status - skip
            int c = byteRx;
            if (!isprint(c))
                c = ' ';
            m_screen->printMidi("dropped %x '%c'\n", byteRx, c);
        }
        else
        {   // start of MIDI message
            switch (byteRx)
            {
                case MidiStatus::timingClock:
                    // timing clock, single byte, dropped
                    break;
                case MidiStatus::activeSensing:
                    // active sensing, single byte, dropped
                    // BUT we abuse the periodic nature of this message
                    // to do a screen update
                    for (int i=0; i<16; i++)
                        (void)m_channelActivity.get(i);
                    if (m_channelActivity.isDirty())
                        show(UpdateScreen);
                    break;
                case MidiStatus::realtimeStart:
                    mvwprintw(m_screen->m_track, 17, 0, "panic on\n");
                    for (int channel=0; channel<16; channel++)
                    {
                        m_midi->putBytes(MidiDevice::FantomOut,
                            MidiStatus::controller|channel, MidiController::allNotesOff, 0);
                        m_midi->putBytes(MidiDevice::FantomOut,
                            MidiStatus::controller|channel, MidiController::resetAllControllers, 0);
                    }
                    break;
                case MidiStatus::realtimeStop:
                    mvwprintw(m_screen->m_track, 17, 0, "panic off\n");
                    break;
                case MidiStatus::sysEx:
                    consumeSysEx(deviceRx);
                    break;
                default:
                {
                    // per-channel status switch
                    uint8_t channelRx = byteRx & 0x0f;
                    switch (byteRx & 0xf0)
                    {
                        case MidiStatus::noteOff:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t velo = m_midi->getByte(deviceRx);
#ifdef LOG_NOTE
                            m_screen->printMidi(
                                "note off %s ch %02x vel %02x\n",
                                m_midi->noteName(note), channelRx, velo);
#endif
                            if (!m_metaMode)
                            {
                                sendEventToFantom(MidiStatus::noteOff, note, velo);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            break;
                        }
                        case MidiStatus::noteOn:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t velo = m_midi->getByte(deviceRx);
#ifdef LOG_NOTE
                            m_screen->printMidi(
                                "note on %s ch %02x vel %02x\n",
                                m_midi->noteName(note), channelRx, velo);
#endif
                            if (m_metaMode)
                            {
                                if (velo > 0)
                                {
                                    changeTrackByNote(note);
                                    toggleInfoMode(note);
                                }
                            }
                            else
                            {
                                sendEventToFantom(MidiStatus::noteOn, note, velo);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            break;
                        }
                        case MidiStatus::aftertouch:
                        {
                            uint8_t note = m_midi->getByte(deviceRx);
                            uint8_t val = m_midi->getByte(deviceRx);
                            m_screen->printMidi(
                                "aftertouch %s ch %02x val %02x\n",
                                m_midi->noteName(note), channelRx, val);
                            sendEventToFantom(MidiStatus::aftertouch, note, val);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        case MidiStatus::controller:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
                            uint8_t val = m_midi->getByte(deviceRx);
#ifdef LOG_CONTROLLER
                            m_screen->printMidi(
                                "controller ch %02x num %02x val %02x\n",
                                channelRx, num, val);
#endif
                            if ((deviceRx == MidiDevice::A30 || deviceRx == MidiDevice::Fcb1010)
                                && (num == MidiController::continuous ||
                                    num == MidiController::modulationWheel ||
                                    num == MidiController::mainVolume ||
                                    num == MidiController::continuousController16 ||
                                    num == MidiController::sustain))
                            {
                                sendEventToFantom(MidiStatus::controller, num, val);
                                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                    show(UpdateScreen);
                            }
                            else if (deviceRx == MidiDevice::A30 && num == 0x5b)
                            {
                                m_metaMode = !!val;
                                m_midi->putBytes(MidiDevice::BcfOut, MidiStatus::controller|0,
                                    0x59, val ? 127 : 0);
                                show(UpdateScreen);
                            }
                            else if (deviceRx == MidiDevice::BcfIn && num == 0x59)
                            {
                                m_metaMode = !!val;
                                show(UpdateScreen);
                            }
                            else if (deviceRx == MidiDevice::BcfIn && num == 0x5b)
                            {
                                m_partOffsetBcf ^= 8;
                                show(UpdateFaders);
                            }
                            else if (deviceRx == MidiDevice::BcfIn
                                    && num >= 0x51 && num <= 0x58)
                            {
                                uint8_t partNum = m_partOffsetBcf + (num - 0x51);
                                FantomPart *p = currentPerf()->m_part+partNum;
                                p->m_vol = val;
                                m_fantom->setVolume(partNum, val);
                                show(UpdateScreen);
                            }
                            else
                            {
                                m_screen->printMidi("dropped controller %02x %02x\n", num, val);
                            }
                            break;
                        }
                        case MidiStatus::programChange:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
#ifdef LOG_PROGRAM_CHANGE
                            m_screen->printMidi(
                                "program change ch %02x num %02x\n",
                                channelRx, num);
#endif
                            if (channelRx == Midi::masterProgramChangeChannel)
                            {
                                if (num == 5)
                                {
                                    m_metaMode = 0;
                                    show(UpdateScreen);
                                }
                                else if (num == 6)
                                {
                                    m_metaMode = 1;
                                    show(UpdateScreen);
                                }
                                else if (currentTrack()->m_chain)
                                {
                                    if (num <= 1)
                                        prevSection();
                                    else if (num >=2)
                                        nextSection();
                                }
                                else
                                    changeSection(num);
                            }
                            break;
                        }
                        case MidiStatus::channelAftertouch:
                        {
                            uint8_t num = m_midi->getByte(deviceRx);
                            m_screen->printMidi(
                                "channelRx pressure channelRx %02x num %02x\n",
                                channelRx, num);
                            sendEventToFantom(MidiStatus::channelAftertouch, num);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        case MidiStatus::pitchBend:
                        {
                            uint8_t num1 = m_midi->getByte(deviceRx);
                            uint8_t num2 = m_midi->getByte(deviceRx);
#ifdef LOG_PITCHBEND
                            m_screen->printMidi(
                                "pitch bend ch %02x val %04x\n",
                                channelRx, (uint16_t)num2<<7|(uint16_t)num1);
#endif
                            sendEventToFantom(MidiStatus::pitchBend, num1, num2);
                            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                                show(UpdateScreen);
                            break;
                        }
                        default:
                            // unknown status, just bail
                            // The next iteration will catch any status-less data.
                            m_screen->printMidi("unknown %02x\n", byteRx);
                            break;
                    } // END per-channel status switch
                } // END default status byte case
            } // END status byte switch
        } // END of MIDI message
        m_screen->flushMidi();
        wnoutrefresh(m_screen->m_track);
        doupdate();
        if (m_metaMode && m_info.m_mode)
            showInfo();
    }
}

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
        SwPart *swPart = currentSection()->m_part[i];
        if (isNoteData)
        {
            if (!swPart->inRange(data1))
            {
                continue;
            }
            int transpose;
            if (swPart->m_transpose < 500)
                transpose = swPart->m_transpose;
            else
                transpose = (swPart->m_transpose - 1000) +
                    swPart->m_customTranspose[
                    (data1 + 12 + swPart->m_customTransposeOffset) % 12];
            data1Out = data1 + transpose;
            if (data1Out > 127)
                data1Out = 127;
            if (swPart->m_mono)
            {
                //swPart->m_monoFilter.m_screen = m_screen;
                if (isNoteOn && !swPart->m_monoFilter
                        .passNoteOn(data1, data2))
                {
                    m_screen->printMidi("note on dropped\n");
                    continue;
                }
                if (isNoteOff && !swPart->m_monoFilter
                        .passNoteOff(data1, data2))
                {
                    m_screen->printMidi("note off dropped\n");
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
                m_screen->printMidi("note on dropped\n");
                continue;
            }
        }
        if (isController)
        {
            if (data1 == MidiController::sustain && swPart->m_mono)
            {
                swPart->m_monoFilter.sustain(data2 != 0);
                m_screen->printMidi("mono sustain\n");
            }
            if (data1 == MidiController::sustain && swPart->m_transposer)
            {
                swPart->m_transposer->setSustain(data2 != 0);
                m_screen->printMidi("transposer sustain\n");
                drop = true;
            }
            swPart->m_controllerRemap->value(data1, data2, &data1Out, &data2Out);
        }
        if (!drop)
        {
            if (data2 != 255)
            {
                m_midi->putBytes(MidiDevice::FantomOut,
                    midiStatus|swPart->m_channel, data1Out, data2Out);
            }
            else
            {
                m_midi->putBytes(MidiDevice::FantomOut,
                    midiStatus|swPart->m_channel, data1Out);
            }
            m_channelActivity.set(swPart->m_channel);
            m_softPartActivity.set(i);
        }
    }
}

void Patcher::show(int updateFlags)
{
    if (updateFlags & UpdateScreen)
    {
        werase(m_screen->m_track);
        wprintw(m_screen->m_track,
            "*** Ray's MIDI patcher " VERSION ", rev " SVN ", " NOW " ***\n\n");
        mvwprintw(m_screen->m_track, 2, 0,
            "track   %03d \"%s\"\nsection %03d/%03d \"%s\"\n",
            1+m_trackIdx, currentTrack()->m_name,
            1+m_sectionIdx, currentTrack()->nofSections(),
            currentSection()->m_name);
        mvwprintw(m_screen->m_track, 2, 44,
            "%03d/%03d within setlist", 1+m_trackIdxWithinSet, m_setList.length());
        mvwprintw(m_screen->m_track, 5, 0,
            "performance \"%s\"\n",
            currentPerf()->m_name);
        mvwprintw(m_screen->m_track, 3, 44,
            "next \"%s\"\n", nextTrackName());
        mvwprintw(m_screen->m_track, 5, 30, "chain mode %s\n",
            currentTrack()->m_chain ?"on":"off");
        if (m_metaMode)
            wattron(m_screen->m_track, COLOR_PAIR(1));
        mvwprintw(m_screen->m_track, 5, 50, "meta mode %s\n",
            m_metaMode?"on":"off");
        if (m_metaMode)
            wattroff(m_screen->m_track, COLOR_PAIR(1));
        int partsShown = 0;
        for (int partIdx=0; partIdx<16;partIdx++)
        {
            int x = (partsShown / 4) * 19;
            int y = 7 + partsShown % 4;
            const FantomPart *part = currentPerf()->m_part+partIdx;
            if (part->m_channel == 255)
                break; // oops, we're not initialised yet
            if (1 || part->m_channel == m_sectionIdx)
            {
                bool active = m_channelActivity.get(part->m_channel);
                if (active)
                    wattron(m_screen->m_track, COLOR_PAIR(1));
                mvwprintw(m_screen->m_track, y, x,
                    "%2d %2d %s ", partIdx+1, 1+part->m_channel, part->m_preset);
                if (active)
                    wattroff(m_screen->m_track, COLOR_PAIR(1));
                partsShown++;
            }
        }
        partsShown = 0;
        const int colLength = 3;
        for (int i=0; i<currentSection()->nofParts(); i++)
        {
            const SwPart *swPart = currentSection()->m_part[i];
            for (int j=0; j<16;j++)
            {
                const FantomPart *hwPart = currentPerf()->m_part+j;
                if (swPart->m_channel == hwPart->m_channel)
                {
                    int x = 0;
                    if (partsShown >= colLength)
                        x += 40;
                    int y = 6 + 5 + 2 + partsShown % colLength;
                    if (partsShown % colLength == 0)
                        mvwprintw(m_screen->m_track, 6+5+1, x,
                            "prt range         patch        vol tps");
                    char keyL[20];
                    keyL[0] = 0;
                    m_midi->noteName(
                        std::max(hwPart->m_keyRangeLower,
                        swPart->m_rangeLower), keyL);
                    char keyU[20];
                    keyU[0] = 0;
                    m_midi->noteName(
                        std::min(hwPart->m_keyRangeUpper,
                        swPart->m_rangeUpper), keyU);
                    int transpose = swPart->m_transpose +
                            (int)hwPart->m_transpose +
                            (int)hwPart->m_oct*12;
                    bool active = m_softPartActivity.get(i);
                    if (active)
                        wattron(m_screen->m_track, COLOR_PAIR(1));
                    assert(hwPart->m_patch.m_name[1] != '[');
                    mvwprintw(m_screen->m_track, y, x,
                        "%3d [%3s - %4s]  %12s %3d %3d", j+1, keyL, keyU,
                        hwPart->m_patch.m_name, hwPart->m_vol,transpose);
                    if (active)
                        wattroff(m_screen->m_track, COLOR_PAIR(1));
                    partsShown++;
                }
            }
        }
    }

    if (updateFlags & UpdateFaders)
        updateBcfFaders();

    if (updateFlags & UpdateFantomDisplay)
    {
        // abuse the fantom screen to print some status info
        char buf[20];
        sprintf(buf, " [ %d / %d ]",
            1+m_sectionIdx, currentTrack()->nofSections());
        m_fantom->setPartName(0, buf);
    }
}

void Patcher::updateBcfFaders(void)
{
    for (int i=0; i<16;i++)
    {
        const FantomPart *hwPart = currentPerf()->m_part+i;
        if (i >= m_partOffsetBcf && i< m_partOffsetBcf + 8)
        {
            int showPart = i - m_partOffsetBcf;
            m_midi->putBytes(MidiDevice::BcfOut, MidiStatus::controller|0,
                0x01+showPart, 0x80 + 4 * hwPart->m_oct);
            m_midi->putBytes(MidiDevice::BcfOut, MidiStatus::controller|0,
                0x51+showPart, hwPart->m_vol);
        }
    }
}

void Patcher::allNotesOff(void)
{
    // send all notes off on all channels in use by the
    // current section
    bool channelsCleared[16];
    for (int i=0; i<16; i++)
        channelsCleared[i] = false;
    for (int i=0; i<currentSection()->nofParts(); i++)
    {
        SwPart *part = currentSection()->m_part[i];
        int channel = part->m_channel;
        if (!channelsCleared[i])
        {
            channelsCleared[i] = true;
            m_midi->putBytes(
                MidiDevice::FantomOut,
                MidiStatus::controller|channel, MidiController::allNotesOff, 0);
            m_midi->putBytes(
                MidiDevice::FantomOut,
                MidiStatus::controller|channel, MidiController::sustain, 0);
            m_screen->printMidi("all notes off ch %02x\n", channel+1);
            part->m_toggler.reset();
        }
    }
}

void Patcher::changeSection(uint8_t sectionIdx)
{
    if (/*sectionIdx >= 0 && */sectionIdx <
        m_trackList[m_trackIdx]->nofSections())

    {
        if (currentSection()->m_noteOffLeave ||
                currentTrack()->m_section[sectionIdx]->m_noteOffEnter)
        {
            allNotesOff();
        }
        m_sectionIdx = sectionIdx;
        show(UpdateScreen|UpdateFantomDisplay);
        redrawwin(m_screen->m_track);
        m_persist.store(m_trackIdx, m_sectionIdx);
    }
}

bool Patcher::debounced(Real delaySeconds)
{
    struct timespec now;
    getTime(&now);
    bool rv = timeDiffSeconds(&m_debouncePrev, &now) > delaySeconds;
    if (rv)
    {
        m_debouncePrev = now;
    }
    return rv;
}

void Patcher::nextSection(void)
{
    if (!debounced(debounceTime))
        return;
    int sectionIdx = m_sectionIdx;
    int newTrack, newSection;
    if (currentSection()->next(&newTrack, &newSection))
    {
        changeTrack(newTrack, UpdateAll);
        changeSection(newSection);
        return;
    }
    sectionIdx++;
    if (sectionIdx >= currentTrack()->nofSections())
        sectionIdx = 0;
    changeSection(sectionIdx);
}

void Patcher::prevSection(void)
{
    if (!debounced(debounceTime))
        return;
    int sectionIdx = m_sectionIdx;
    int newTrack, newSection;
    if (currentSection()->previous(&newTrack, &newSection))
    {
        changeTrack(newTrack, UpdateAll);
        changeSection(newSection);
        return;
    }
    if (sectionIdx == 0)
        sectionIdx = currentTrack()->nofSections()-1;
    else
        sectionIdx--;
    changeSection(sectionIdx);
}

void Patcher::changeTrack(uint8_t track, int updateFlags)
{
    //m_screen->printMidi("change track %d\n", track);
    m_trackIdx = track;
    m_sectionIdx = currentTrack()->m_startSection; // cannot use changeSection!
    m_midi->putBytes(MidiDevice::FantomOut,
        MidiStatus::programChange|Fantom::programChangeChannel, (uint8_t)m_trackIdx);
    show(updateFlags);
    m_persist.store(m_trackIdx, m_sectionIdx);
}

void Patcher::toggleInfoMode(uint8_t note)
{
    if (note == MetaNote::info)
    {
        m_info.m_mode = !m_info.m_mode;
        if (m_info.m_mode)
        {
            m_info.m_text = getNetworkInterfaceAddresses();
        }
    }
}

void Patcher::showInfo()
{
    struct timespec now;
    getTime(&now);
    bool rv = timeDiffSeconds(&m_info.m_previous, &now) > 0.333;
    if (rv)
    {
        m_info.m_previous = now;
        // abuse the fantom screen to print some info
        char buf[20];
        for (size_t i=0, j=m_info.m_offset; i<sizeof(buf); i++)
        {
            j++;
            if (j >= m_info.m_text.length())
                j = 0;
            buf[i] = m_info.m_text[j];
        }
        m_fantom->setPartName(0, buf);
        m_info.m_offset++;
        if (m_info.m_offset >= m_info.m_text.size())
            m_info.m_offset = 0;
    }
}

void Patcher::changeTrackByNote(uint8_t note)
{
    bool valid = false;
    if (note == MetaNote::up && m_trackIdxWithinSet < m_setList.length()-1)
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
}

void Patcher::consumeSysEx(int device)
{
    m_screen->printMidi("sysEx ");
    for (;;)
    {
        uint8_t byteRx = m_midi->getByte(device);
        int c = byteRx;
        if (!isprint(c))
            c = ' ';
        m_screen->printMidi("%x '%c' ", byteRx, c);
        if (byteRx == MidiStatus::EOX)
            break;
    }
    m_screen->printMidi("\n");
}

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        const char *dir = argv[1];
        if (-1 == chdir(dir))
        {
            fprintf(stderr, "cannot chdir to %s\n", dir);
            return -3;
        }
    }
#if 0
    std::vector <Track *>m_trackList;
    importTracks(TRACK_DEF, m_trackList);
#ifdef EXPORT_TRACKS
    exportTracks(m_trackList, "tracks_out.xml");
#endif
    clearTracks(m_trackList);
    return 0;
#endif

    int rv = setAlarmHandler();
    if (rv < 0)
        return rv;

    Screen screen;
    try
    {
        wprintw(screen.m_track,
            "*** initialising ***\n\n");
        wrefresh(screen.m_track);
        Midi midi(&screen);
        Fantom fantom(&screen, &midi);
        Patcher patcher(&screen, &midi, &fantom);
        patcher.loadTrackDefs();
        patcher.downloadPerfomanceData();
        patcher.mergePerformanceData();
        //patcher.dumpTrackList();
        patcher.show(Patcher::UpdateScreen|Patcher::UpdateFaders);
        patcher.eventLoop();
    }
    catch (Error &e)
    {
        endwin();
        fprintf(stderr, "** %s\n", e.m_message);
        return -1;
    }
    catch (...)
    {
        endwin();
        fprintf(stderr, "unhandled exception\n");
    }
    return -2;
}
