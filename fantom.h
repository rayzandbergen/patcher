#ifndef FANTOM_H
#define FANTOM_H
#include <stdint.h>
#include "screen.h"
#include "midi.h"
#include "dump.h"

class FantomPatch
{
public:
    char m_name[20];
    void save(const Dump *d) { d->save((const char*)m_name); }
    void restore(const Dump *d) { d->restore((char*)m_name); }
};

class FantomPart
{
public:
    uint8_t m_number;
    uint8_t m_channel;
    uint8_t m_bankSelectMsb;
    uint8_t m_bankSelectLsb;
    uint8_t m_pCh;
    char m_preset[20];
    uint8_t m_vol;
    int8_t m_transpose;
    int8_t m_oct;
    uint8_t m_keyRangeLower;
    uint8_t m_keyRangeUpper;
    uint8_t m_fadeWidthLower;
    uint8_t m_fadeWidthUpper;
    FantomPatch m_patch;
    void constructPreset(bool &patchReadAllowed);
    void save(const Dump *d);
    void restore(const Dump *d);
    void dumpToLog(Screen *screen, const char *prefix) const;
};

class FantomPerformance
{
public:
    char m_name[20];
    FantomPart m_part[16];
    void save(const Dump *d);
    void restore(const Dump *d);
};

struct Fantom
{
    static const uint8_t programChangeChannel = 0x0f;
    static const int nameSize = 12;
    Screen *m_screen;
    Midi *m_midi;
    Fantom(Screen *screen, Midi *midi): m_screen(screen), m_midi(midi) { }
    void setParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void getParam(const uint32_t addr, const uint32_t length, uint8_t *data);
    void setVolume(uint8_t part, uint8_t val);
    void getPartParams(FantomPart *p, int idx);
    void getPatchName(char *s, int idx);
    void getPerfName(char *s);
    void setPerfName(const char *s);
    void setPartName(int part, const char *s);
};
#endif // FANTOM_H
