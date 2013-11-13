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
class Screen
{
    enum Id { track, midi, debug };
	WINDOW *m_midiLog;
    int vprintf(Id id, const char *fmt, va_list);
    FILE *m_fpLog;
    bool m_enableMidiLog;
public:
	WINDOW *m_track;
    int printMidi(const char *fmt, ...);
    void flushMidi();
    int printLog(const char *fmt, ...);
    void dumpToLog(const uint8_t *data, int n);
	Screen();
	~Screen();
};
#endif
