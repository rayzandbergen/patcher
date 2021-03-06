/*! \file screen.h
 *  \brief Contains an object that wraps a curses(3X) screen.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef SCREEN_H
#define SCREEN_H
#define GCC_PRINTF
#ifdef CURSES
#include <cstddef> // for NULL
#include <cursesw.h>
#elif NCURSES
#include <ncursesw/ncurses.h>
#else
#error neither CURSES nor NCURSES defined
#endif
#include <stdint.h>
#include "patchercore.h"
//!  \brief Wraps ncurses(3X) screens "main" and "log" in an object with some convenience functions.
class Screen
{
    WINDOW *m_mainWindow;       //!<    The main window.
    WINDOW *m_logWindow;        //!<    The log window.
public:
    //! \brief Return the main window.
    WINDOW *main() const { return m_mainWindow; }
    //! \brief Return the log window.
    WINDOW *log() const { return m_logWindow; }
    static void showProgressBar(WINDOW *win, int y, int x, Real f); //!< show a progress bar at screen coordinates (x,y).
    static void fprintfBinaryString(FILE *fp, const uint8_t *data, int n);
    Screen(bool enableMain = true, bool enableLog = false);
    ~Screen();
};
#endif
