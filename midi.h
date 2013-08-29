#ifndef MIDI_H
#define MIDI_H
#include <stdint.h>
#include "screen.h"

// MIDI protocol: status byte
namespace MidiStatus
{
    enum StatusByte
    {
        noteOff = 0x80,
        noteOn = 0x90,
        aftertouch = 0xa0,
        controller = 0xb0,
        programChange = 0xc0,
        channelAftertouch = 0xd0,
        pitchBend = 0xe0,
        sysEx = 0xf0,
        tuneRequest = 0xf6,
        EOX = 0xf7,
        timingClock = 0xf8,
        realtimeStart = 0xfa,
        realtimeContinue = 0xfb,
        realtimeStop = 0xfc,
        activeSensing = 0xfe,
        sysReset = 0xff
    };
};

// MIDI protocol: controller numbers
namespace MidiController
{
    enum DataByte
    {
        continuous = 0,
        modulationWheel = 1,
        dataEntry = 6,
        mainVolume = 7,
        continuousController16 = 16, // FCB1010 right pedal
        sustain = 0x40,
        dataEntryInc = 0x60,
        dataEntryDec = 0x61,
        resetAllControllers = 121,
        allNotesOff = 123
    };
};

// My hardware
struct MidiDevice 
{
    enum { none = 0, A30, Fcb1010, FantomOut, FantomIn, BcfOut, BcfIn, max, all };
    const char *m_cardName;
    int m_card;
    int m_sub1;
    int m_sub2;
    int m_direction;
    int m_id;
    const char *m_shortDescr;
    const char *m_longDescr;
    int m_fd;
};

class Midi {
    Screen *m_screen;
	struct MidiDevice *m_deviceList;
    int m_deviceIdToDeviceTabIdx[MidiDevice::max];
    int fd(int deviceId) const;
    int cardNameToNum(const char *target) const;
	int openRaw(const char *portName, int mode) const;
    void openDevices(void);
public:
    static const uint8_t masterProgramChangeChannel = 0x0f;
	Midi(Screen *screen);
	~Midi(void);
    int wait(int usecTimeout = 0, int device = MidiDevice::all) const;
	uint8_t getByte(int device);
	void putByte(int device, uint8_t b1);
    void putBytes(int device, const uint8_t *b, int n);
	void putBytes(int device, uint8_t b1, uint8_t b2);
	void putBytes(int device, uint8_t b1, uint8_t b2, uint8_t b3);
    static const char *noteName(uint8_t num);
    static void noteName(uint8_t num, char *s);
    static bool isNote(uint8_t status);
    static bool isNoteOn(uint8_t status, uint8_t data1, uint8_t data2);
    static bool isNoteOff(uint8_t status, uint8_t data1, uint8_t data2);
    static bool isController(uint8_t status);
};

#endif // MIDI_H
