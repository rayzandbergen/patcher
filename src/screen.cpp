/*! \file screen.cpp
 *  \brief Contains an object that wraps a curses(3X) screen.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "screen.h"
#include "error.h"

Screen::Screen(): m_enableMidiLog(true)
{
    int lines = 25;
#ifdef RASPBIAN
    m_enableMidiLog = false;
#endif
    m_fpLog = NULL;
#if 0
    int columns = 80;
    const char *s;
    s = getenv("LINES");
    if (s == NULL || sscanf(s, "%d", &lines) != 1)
        lines = 25;
    s = getenv("COLUMNS");
    if (s == NULL || sscanf(s, "%d", &columns) != 1)
        columns = 80;
#endif
    initscr();
    start_color();
    curs_set(0);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    const int trackWinHeight = 16;
    m_track = newwin(trackWinHeight, 80, 0, 0);
    wrefresh(m_track);
    if (m_enableMidiLog)
    {
        m_midiLog = newwin(lines-trackWinHeight-2, 40, trackWinHeight+1, 4);
        scrollok(m_midiLog, true);
    }
    else
        m_midiLog = NULL;
}

Screen::~Screen()
{
    if (m_enableMidiLog)
        delwin(m_midiLog);
    delwin(m_track);
    endwin();
}

int Screen::printLog(const char *fmt, ...)
{
    if (!m_fpLog)
    {
        m_fpLog = fopen("log.txt", "w");
        if (!m_fpLog)
            throw(Error("fopen", errno));
    }
    va_list ap;
    va_start(ap, fmt);
    int rv = vfprintf(m_fpLog, fmt, ap);
    va_end(ap);
    return rv;
}

void Screen::dumpToLog(const uint8_t *data, int n)
{
    for (int i=0;i<n;i++)
    {
        if (!isprint(data[i]))
            printLog("\\%02x", data[i]);
        else
            printLog("%c", data[i]);
    }
    printLog("\n");
}

int Screen::printMidi(const char *fmt, ...)
{
    if (!m_enableMidiLog)
        return 0;
    va_list ap;
    va_start(ap, fmt);
    int rv = vprintf(midi, fmt, ap);
    va_end(ap);
    return rv;
}

void Screen::flushMidi()
{
    if (m_enableMidiLog)
        wrefresh(m_midiLog);
}

int Screen::vprintf(Id id, const char *fmt, va_list ap)
{
    switch (id)
    {
        case track:
            return vwprintw(m_track, fmt, ap);
        case midi:
        default:
            return vwprintw(m_midiLog, fmt, ap);
    }
}

void Screen::showProgressBar(WINDOW *win, int y, int x, Real fraction)
{
    const int width = 30;
    mvwaddch(win, y, x++, '[');
    for (int i=0; i<width; i++)
    {
        mvwaddch(win, y, x, i < (int)(fraction*width) ? '=' : ' ' );
        x++;
    }
    mvwaddch(win, y, x++, ']');
    mvwprintw(win, y, x, " %3.0f%%", (double)((Real)100.0 * fraction));
}
