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
    enum Id                         //!< Screen ID for logging.
        { track, midi, debug };
	WINDOW *m_midiLog;              //!< curses MIDI window object.
    int vprintf(Id id, const char *fmt, va_list);   //!< Low level log function.
    FILE *m_fpLog;                  //!< FILE pointer for witten logs.
    bool m_enableMidiLog;           //!< If true: enable the MIDI log screen.
public:
	WINDOW *m_track;                //!< curses track info window.
    int printMidi(const char *fmt, ...); //!< printf-like MIDI log function.
    void flushMidi();               //!< Update the MIDI log screen.
    int printLog(const char *fmt, ...); //!< printf-like log function.
    void dumpToLog(const uint8_t *data, int n); //!< dump a binary string to the log.
    static void showProgressBar(WINDOW *win, int y, int x, Real f); //!< show a progress bar at screen coordinates.
	Screen();                       //!< Constructor.
	~Screen();                      //!< Destructor.
};
#endif
