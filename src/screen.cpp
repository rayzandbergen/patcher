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

/*! \brief Construct a Screen object.
 *
 * \param[in]       enableMain  Enable the main window.
 * \param[in]       enableLog   Enable the log window.
 */
Screen::Screen(bool enableMain, bool enableLog):m_mainWindow(0), m_logWindow(0)
{
    if (!enableMain && !enableLog)
        return;

    const int lines = 25;
    int trackWinHeight = 0;
    initscr();
    start_color();
    curs_set(0);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
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

//! \brief Destructor.
Screen::~Screen()
{
    if (m_mainWindow)
        delwin(m_mainWindow);
    if (m_logWindow)
        delwin(m_logWindow);
    endwin();
}


/*! \brief  Print a binary string to a FILE as a hex string.
 *
 *  \param[in]  fp      FILE to write to.
 *  \param[in]  data    Start of binary data.
 *  \param[in]  n       Size of binary data.
 */
void Screen::fprintfBinaryString(FILE *fp, const uint8_t *data, int n)
{
    for (int i=0;i<n;i++)
    {
        if (!isprint(data[i]))
            fprintf(fp, "\\%02x", data[i]);
        else
            fprintf(fp, "%c", data[i]);
    }
    fprintf(fp, "\n");
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
