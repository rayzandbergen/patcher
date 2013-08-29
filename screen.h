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
	WINDOW *m_midiLog;
    int vprintf(int id, const char *fmt, va_list);
    FILE *m_fpLog;
    bool m_enableMidiLog;
public:
    enum Id { track, midi, debug };
	WINDOW *m_track;
    int printMidi(const char *fmt, ...);
    void flushMidi();
    int printLog(const char *fmt, ...);
    void dumpToLog(const uint8_t *data, int n);
	Screen(int argc, char **argv);
	~Screen();
};
#endif
