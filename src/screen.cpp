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

Screen::Screen(bool enableMain, bool enableLog):m_mainWindow(0), m_logWindow(0)
{
    const int lines = 25;
    int trackWinHeight = 0;
    m_fpLog = 0;
    initscr();
    start_color();
    curs_set(0);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    if (enableMain)
    {
        trackWinHeight = 16;
        m_mainWindow = newwin(trackWinHeight, 80, 0, 0);
        wrefresh(m_mainWindow);
    }
    if (enableLog)
    {
        m_logWindow = newwin(lines-trackWinHeight-2, 40, trackWinHeight+1, 4);
        scrollok(m_logWindow, true);
    }
}

Screen::~Screen()
{
    if (m_mainWindow)
        delwin(m_mainWindow);
    if (m_logWindow)
        delwin(m_logWindow);
    endwin();
}

void Screen::openLog()
{
    if (!m_fpLog)
    {
        m_fpLog = fopen("log.txt", "w");
        if (!m_fpLog)
            throw(Error("fopen", errno));
    }
}

int Screen::printLog(const char *fmt, ...)
{
    openLog();
    va_list ap;
    va_start(ap, fmt);
    int rv = vfprintf(m_fpLog, fmt, ap);
    va_end(ap);
    return rv;
}

void Screen::dumpToLog(const uint8_t *data, int n)
{
    openLog();
    for (int i=0;i<n;i++)
    {
        if (!isprint(data[i]))
            fprintf(m_fpLog, "\\%02x", data[i]);
        else
            fprintf(m_fpLog, "%c", data[i]);
    }
    fprintf(m_fpLog, "\n");
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
