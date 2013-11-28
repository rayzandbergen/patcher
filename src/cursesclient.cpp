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
#define VERSION "1.2.0"     //!< global version number

//! \brief Screen logger object.
class QueueListener
{
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
    const char *nextTrackName() {
        if (m_trackIdxWithinSet+1 < m_setList.size())
            return m_trackList[m_setList[m_trackIdxWithinSet+1]]->m_name;
        else
            return "<none>";
    }
    void loadTrackDefs();
    void mergePerformanceData();
    void updateScreen();
    void eventLoop();
    QueueListener(Screen *s):
        m_channelActivity(Midi::NofChannels, Midi::Note::max),
        m_softPartActivity(64 /*see tracks.xsd*/, Midi::Note::max),
        m_screen(s),
        m_trackIdx(0), m_trackIdxWithinSet(0), m_sectionIdx(0),
        m_metaMode(false), m_nofScreenUpdates(0)
    {
        m_eventRxQueue.openRead();
    }
};

void QueueListener::updateScreen()
{
    m_nofScreenUpdates++;
    werase(m_screen->main());
    wprintw(m_screen->main(),
        "*** Ray's MIDI patcher " VERSION ", rev " SVN ", " NOW " ***\n\n");
#if 0
    for (int i=0; i<16; i++)
    {
        mvwprintw(m_screen->main(), 1, i*4, "%02d ", m_channelActivity.m_triggerCount[i]);
    }
#else
    mvwprintw(m_screen->main(), 1, 63, "%08d", m_nofScreenUpdates);
#endif
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
    //bool partActivity[Fantom::Performance::NofParts];
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

void QueueListener::loadTrackDefs()
{
    Event event;
    m_eventRxQueue.receive(event);
    importTracks(TRACK_DEF, m_trackList, m_setList);
}

void QueueListener::eventLoop()
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
            //timeSum(nextTime, nextTime, TimeSpec((Real)100.0));
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
            wprintw(m_screen->log(), "timeout\n");
            if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
            {
                updateScreen();
                wrefresh(m_screen->main());
            }
        }
        else
        {
            wprintw(m_screen->log(), "%s\n", event.toString().c_str());
            m_metaMode = !!event.m_metaMode;
            if (event.m_currentTrack != Event::Unknown)
                m_trackIdx = event.m_currentTrack;
            if (event.m_currentSection != Event::Unknown)
                m_sectionIdx = event.m_currentSection;
            if (event.m_trackIdxWithinSet != Event::Unknown)
                m_trackIdxWithinSet = event.m_trackIdxWithinSet;
            if ((event.m_type == Event::MidiOut3Bytes ||
                 event.m_type == Event::MidiOut2Bytes ||
                 event.m_type == Event::MidiOut1Byte) &&
                event.m_deviceId == Midi::Device::FantomOut)
            {
                wrefresh(m_screen->log());
                if (Midi::isNote(event.m_midi[0]))
                {
                    uint8_t channel = event.m_midi[0] & 0x0f;
                    m_channelActivity.trigger(channel, event.m_midi[1], m_eventRxTime);
                    m_softPartActivity.trigger(event.m_part, event.m_midi[1], m_eventRxTime);
                }
                else if (Midi::isController(event.m_midi[0]))
                {
                    uint8_t channel = event.m_midi[0] & 0x0f;
                    m_channelActivity.trigger(channel, 0, m_eventRxTime);
                    m_softPartActivity.trigger(event.m_part, 0, m_eventRxTime);
                }
                if (m_channelActivity.isDirty() || m_softPartActivity.isDirty())
                {
                    updateScreen();
                    wrefresh(m_screen->main());
                }
            }
            else
            {
                updateScreen();
                wrefresh(m_screen->main());
            }
        }
    }
}

void QueueListener::mergePerformanceData()
{
    Fantom::PerformanceList performanceList;
    const char *cacheFileName = "fantom_cache.bin";
    if (performanceList.readFromCache(cacheFileName) != nofTracks())
    {
        throw(Error("cannot read fantom cache"));
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

int main(void)
{
#if 0
    Event m;
    std::cout << sizeof(m) << "\n";
    return 0;
#endif
    try
    {
        Screen screen(true, true);
        QueueListener q(&screen);
        q.loadTrackDefs();
        q.mergePerformanceData();
        q.eventLoop();
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
