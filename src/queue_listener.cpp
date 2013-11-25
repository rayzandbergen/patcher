#include <queue.h>
#include <iostream>
#include "screen.h"
#include "trackdef.h"
#include "xml.h"
#include "error.h"
#include "patcher.h"

class QueueListener
{
    Screen *m_screen;
    Queue m_queue;
    TrackList m_trackList;              //!< Global \a Track list.
    SetList m_setList;                  //!< Global \a SetList object.
public:
    void loadTrackDefs();
    void eventLoop();
    QueueListener(Screen *s): m_screen(s)
    {
    }
};

void QueueListener::loadTrackDefs()
{
    importTracks(TRACK_DEF, m_trackList, m_setList);
}

void QueueListener::eventLoop()
{
    LogMessage msg;
    wprintw(m_screen->main(), "q.getattr: %s\n", m_queue.getAttr().c_str());
    wprintw(m_screen->main(), "sizeof msg: %d\n", (int)sizeof(msg));
    wrefresh(m_screen->main());
    for (;;)
    {
        m_queue.receive(msg);
        wprintw(m_screen->log(), "%s\n", msg.toString().c_str());
        wrefresh(m_screen->log());
    }
}

int main(void)
{
#if 0
    LogMessage m;
    std::cout << sizeof(m) << "\n";
    return 0;
#endif
    try
    {
        Screen screen(true, true);
        QueueListener q(&screen);
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
