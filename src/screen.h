/*! \file screen.h
 *  \brief Contains an object that wraps a curses(3X) screen.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef SCREEN_H
#define SCREEN_H
#define GCC_PRINTF
#ifdef RASPBIAN
#include <cstddef> // for NULL
#include <cursesw.h>
#else
#include <ncursesw/ncurses.h>
#endif
#include <stdint.h>
#include "patcher.h"
//!  \brief Wraps an ncurses(3X) screen in an object with some convenience functions.
class Screen
{
    WINDOW *m_mainWindow;
    WINDOW *m_logWindow;
    FILE *m_fpLog;                  //!< FILE pointer for witten logs.
    void openLog();
public:
    WINDOW *log() const { return m_logWindow; }
    WINDOW *main() const { return m_mainWindow; }
    int printLog(const char *fmt, ...); //!< printf-like log function.
    void dumpToLog(const uint8_t *data, int n); //!< dump a binary string to the log.
    static void showProgressBar(WINDOW *win, int y, int x, Real f); //!< show a progress bar at screen coordinates.
	Screen(bool enableMain, bool enableLog);                       //!< Constructor.
	~Screen();                      //!< Destructor.
};
#endif
