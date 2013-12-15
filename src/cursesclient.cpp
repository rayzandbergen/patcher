#include <queue.h>
#include <iostream>
#include "screen.h"
#include "trackdef.h"
#include "fantomdef.h"
#include "xml.h"
#include "error.h"
#include "patchercore.h"
#include "timestamp.h"
#include "now.h"
#include "activity.h"
#define VERSION "1.4.0"     //!< global version number

//! \brief A curses client for the patcher-core.
class CursesClient
{
    FILE *m_fpLog;                      //!< Log stream.
    XML *m_xml;                         //!< XML parser.
    ActivityList m_channelActivity;     //!< Per-channel \a Activity.
    ActivityList m_softPartActivity;    //!< Per-\a SwPart \a Activity.
    Screen *m_screen;                   //!< Screen object to log to.
    Queue m_eventRxQueue;               //!< Event RX queue.
    TrackList m_trackList;              //!< Global \a Track list.
    Fantom::PerformanceList m_performanceList; //!< Performance list.
    SetList m_setList;                  //!< Global \a SetList object.
    int m_trackIdx;                     //!< Current track index
    int m_trackIdxWithinSet;            //!< Current track index within \a SetList.
    int m_sectionIdx;                   //!< Current section index.
    bool m_metaMode;                    //!< Meta mode switch.
    TimeSpec m_eventRxTime;             //!< Arrival time of the current Midi event.
    int m_nofScreenUpdates;             //!< Screen update counter.
public:
    size_t nofTracks() const { return m_trackList.size(); } //!< The number of \a Tracks.
    Track *currentTrack() const {
        return m_trackList[m_trackIdx]; } //!< The current \a Track.
    Section *currentSection() const {
        return currentTrack()->m_sectionList[m_sectionIdx]; } //!< The current \a Section.
    //! \brief The name of the next track in the setlist.
    const char *nextTrackName() {
        if (m_trackIdxWithinSet+1 < m_setList.size())
            return m_trackList[m_setList[m_trackIdxWithinSet+1]]->m_name;
        else
            return "<none>";
    }
    //! \brief process All Notes Off event for a given channel.
    void allNotesOff(uint8_t channel);
    //! \brief process All Notes Off event.
    void allNotesOff();
    //! \brief Load track list and performance list and merge performance data.
    void loadConfig();
    //! \brief Set or unset screen attributes according to activity state.
    void updateActivityAttributes(ActivityList::State s, bool enable);
    //! \brief Write to the curses screen.
    void updateScreen();
    //! \brief Event loop, never returns.
    void eventLoop();
    //! \brief Constructs a curses client.
    CursesClient(bool enableLogging, Screen *s):
        m_fpLog(0),
        m_xml(0),
        m_channelActivity(Midi::NofChannels, Midi::Note::max),
        m_softPartActivity(64 /*see tracks.xsd*/, Midi::Note::max),
        m_screen(s),
        m_trackIdx(0), m_trackIdxWithinSet(0), m_sectionIdx(0),
        m_metaMode(false), m_nofScreenUpdates(0)
    {
        if (enableLogging)
            m_fpLog = fopen("clientlog.txt", "wb");
        m_xml = new XML;
        m_eventRxQueue.openRead();
    }
};

void CursesClient::updateActivityAttributes(ActivityList::State s, bool enable)
{
    if (enable)
    {
        switch(s)
        {
            case ActivityList::event:
                wattron(m_screen->main(), A_UNDERLINE);
                wattron(m_screen->main(), COLOR_PAIR(1));
                break;
            case ActivityList::on:
                wattron(m_screen->main(), COLOR_PAIR(1));
                break;
            default:
                ;
        }
    }
    else
    {
        switch(s)
        {
            case ActivityList::event:
                wattroff(m_screen->main(), A_UNDERLINE);
                wattroff(m_screen->main(), COLOR_PAIR(1));
                break;
            case ActivityList::on:
                wattroff(m_screen->main(), COLOR_PAIR(1));
                break;
            default:
                ;
        }
    }
}

void CursesClient::updateScreen()
{
    m_nofScreenUpdates++;
    werase(m_screen->main());
    wprintw(m_screen->main(),
        "*** Ray's MIDI patcher " VERSION ", rev " SVN ", " NOW " ***\n\n");
    if (m_fpLog)
    {
        fprintf(m_fpLog, "screen update %08d\n", m_nofScreenUpdates);
        fprintf(m_fpLog, "activity ");
        for (int i=0; i<Midi::NofChannels; i++)
            fprintf(m_fpLog, "%02d ", m_channelActivity.triggerCount(i));
        fprintf(m_fpLog, "\n");
    }
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
    for (int partIdx=0; partIdx<Fantom::Performance::NofParts;partIdx++)
    {
        int x = (partsShown / 4) * 19;
        int y = 7 + partsShown % 4;
        const Fantom::Part *part = currentTrack()->m_performance->m_partList+partIdx;
        ASSERT(part->m_channel != 255);
        if (1 || part->m_channel == m_sectionIdx)
        {
            ActivityList::State s = m_channelActivity.get(part->m_channel);
            updateActivityAttributes(s, true);
            mvwprintw(m_screen->main(), y, x,
                "%2d %2d %s ", partIdx+1, 1+part->m_channel, part->m_preset);
            updateActivityAttributes(s, false);
            partsShown++;
        }
    }
    partsShown = 0;
    const int colLength = 3;
    for (size_t i=0; i<currentSection()->m_partList.size(); i++)
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
                        (int)hwPart->m_octave*12;
                ActivityList::State s = m_softPartActivity.get(i);
                updateActivityAttributes(s, true);
                ASSERT(hwPart->m_patch.m_name[1] != '[');
                mvwprintw(m_screen->main(), y, x,
                    "%3d [%3s - %4s]  %12s %3d %3d", j+1, keyL, keyU,
                    hwPart->m_patch.m_name, hwPart->m_volume,transpose);
                updateActivityAttributes(s, false);
                partsShown++;
            }
        }
    }
}

void CursesClient::allNotesOff(uint8_t channel)
{
    m_channelActivity.clear(channel);
    for (size_t pIdx = 0; pIdx < currentSection()->m_partList.size(); pIdx++)
    {
        const SwPart *swPart = currentSection()->m_partList[pIdx];
        if (swPart->m_channel == channel)
            m_softPartActivity.clear(pIdx);
    }
}

void CursesClient::allNotesOff()
{
    m_channelActivity.clear();
    m_softPartActivity.clear();
}

void CursesClient::eventLoop()
{
    Event event;
    wprintw(m_screen->main(), "q.getattr: %s\n", m_eventRxQueue.getAttr().c_str());
    wprintw(m_screen->main(), "sizeof event: %d\n", (int)sizeof(event));
    wrefresh(m_screen->main());
    for (;;)
    {
        TimeSpec nextTime;
        bool timedOut = false;
        if (m_channelActivity.nextExpiry(nextTime))
        {
            timedOut = !m_eventRxQueue.receive(event, nextTime);
        }
        else
        {
            m_eventRxQueue.receive(event);
        }
        getTime(m_eventRxTime);
        m_channelActivity.update(m_eventRxTime);
        m_softPartActivity.update(m_eventRxTime);
        if (timedOut)
        {
            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
            {
                updateScreen();
                wrefresh(m_screen->main());
            }
        }
        else
        {
            if (m_fpLog)
            {
                fprintf(m_fpLog, "%s\n", event.toString().c_str());
            }
            m_metaMode = !!event.m_metaMode;
            bool doAllNotesOff = false;
            if (event.m_currentTrack != Event::Unspecified && m_trackIdx != event.m_currentTrack)
            {
                m_trackIdx = event.m_currentTrack;
                doAllNotesOff = true;
            }
            if (event.m_currentSection != Event::Unspecified && m_sectionIdx != event.m_currentSection)
            {
                m_sectionIdx = event.m_currentSection;
                doAllNotesOff = true;
            }
            if (doAllNotesOff)
            {
                allNotesOff();
            }
            if (event.m_trackIdxWithinSet != Event::Unspecified)
                m_trackIdxWithinSet = event.m_trackIdxWithinSet;
            if ((event.m_type == Event::MidiOut3Bytes ||
                 event.m_type == Event::MidiOut2Bytes ||
                 event.m_type == Event::MidiOut1Byte) &&
                event.m_deviceId == Midi::Device::FantomOut)
            {
                // parse MIDI out event
                uint8_t channel = Midi::channel(event.m_midi[0]);
                if (Midi::isNote(event.m_midi[0]))
                {
                    bool on = Midi::isNoteOn(event.m_midi[0], event.m_midi[1], event.m_midi[2]);
                    m_channelActivity.trigger(channel, event.m_midi[1], on, m_eventRxTime);
                    if (event.m_part != Event::Unspecified)
                        m_softPartActivity.trigger(event.m_part, event.m_midi[1], on, m_eventRxTime);
                }
                else if (Midi::isController(event.m_midi[0]) || (Midi::status(event.m_midi[0]) == Midi::pitchBend))
                {
                    if (event.m_midi[1] == Midi::allNotesOff)
                    {
                        allNotesOff(channel);
                    }
                    else
                    {
                        // Abuse note C0 to store regular controller activity.
                        m_channelActivity.trigger(channel, Midi::Note::C0, false, m_eventRxTime);
                        if (event.m_part != Event::Unspecified)
                            m_softPartActivity.trigger(event.m_part, Midi::Note::C0, false, m_eventRxTime);
                    }
                }
                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                {
                    updateScreen();
                    wrefresh(m_screen->main());
                }
            }
            else
            {
                if (m_fpLog)
                    fprintf(m_fpLog, "ignored %s\n", event.toString().c_str());
                updateScreen();
                wrefresh(m_screen->main());
            }
        }
        if (m_fpLog)
            fflush(m_fpLog);
    }
}

void CursesClient::loadConfig()
{
    m_xml->importTracks(TRACK_DEF, m_trackList, m_setList);
    // Wait for core process to start its event loop.
    Event event;
    m_eventRxQueue.receive(event);
    // By now the cache file should be available.
    m_xml->importPerformances("fantom_cache.xml", m_performanceList);
    for (size_t t = 0; t < nofTracks(); t++)
    {
        Track *track = m_trackList[t];
        track->m_performance = m_performanceList[t];
        if (m_fpLog)
            fprintf(m_fpLog, "merging performance %s\n", track->m_performance->m_name);
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
                        if (m_fpLog)
                            fprintf(m_fpLog, "merging part %s\n", hwPart->m_preset);
                    }
                }
            }
        }
    }
}

int main(int argc, char**argv)
{
    bool enableLogging = true;
    for (;;)
    {
        int opt = getopt(argc, argv, "lh");
        if (opt == -1)
            break;
        switch (opt)
        {
            case 'l':
                enableLogging = true;
                break;
            default:
                std::cerr << "\ncursesclient [-h|?] [-l]\n\n"
                    "  -h|?     This message\n"
                    "  -l       Enable logging\n\n";
                return 1;
                break;
        }
    }
    if (argc > optind)
    {
        std::cerr << "unrecognised trailing arguments, try -h\n";
    }
    try
    {
        Screen screen;
        CursesClient cursesClient(enableLogging, &screen);
        cursesClient.loadConfig();
        cursesClient.eventLoop();
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
