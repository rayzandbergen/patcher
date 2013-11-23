/*! \file midi.h
 *  \brief Contains objects that handle all MIDI traffic.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef MIDI_H
#define MIDI_H
#include <stdint.h>
#include "screen.h"

//! \brief Namespace for all MIDI driver objects.
namespace Midi
{

const int NofChannels = 16; //!< The number of MIDI channels.

//! \brief MIDI protocol: status bytes.
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

//! \brief MIDI protocol: controller numbers.
enum DataByte
{
    continuous = 0,
    modulationWheel = 1,
    BCFSpinner1 = 1,
    dataEntry = 6,
    mainVolume = 7,
    continuousController16 = 16, //!< FCB1010 right pedal
    sustain = 0x40,
    BCFFader1 = 0x51,
    BCFFader8 = 0x58,
    BCFSwitchA = 0x59,
    effects1Depth = 0x5b, //!< Reverb depth
    dataEntryInc = 0x60,
    dataEntryDec = 0x61,
    resetAllControllers = 121,
    allNotesOff = 123
};

//! \brief Direction of MIDI data, as seen from Pi.
enum InOut { none, in = 100, out };

//! \brief State of a single MIDI device.
struct Device
{
    //!<    \brief My hardware.
    enum DeviceId { none = 0, A30, Fcb1010, FantomOut, FantomIn, BcfOut, BcfIn, max, all };
    const char *m_cardName;     //!< ALSA driver name.
    int m_card;                 //!< ALSA card number.
    int m_sub1;                 //!< ALSA sub ID 1.
    int m_sub2;                 //!< ALSA sub ID 2.
    InOut m_direction;          //!< In or out.
    DeviceId m_id;              //!< Device ID.
    const char *m_shortDescription;   //!< Short description.
    const char *m_longDescription;    //!< Long description.
    int m_fd;                   //!< ALSA File descriptor.
};

/*! \brief Handles all MIDI traffic, including logging.
 *
 * It uses ALSA drivers to obtain a file descriptor for each MIDI
 * device. After initialisation ALSA is no longer used, as the
 * standard POSIX interface for file descriptors is rich enough.
 * This object logs to a \a Screen object.
 */
class Driver {
    WINDOW *m_window;
	struct Device *m_deviceList;                    //!< List of all MIDI devices.
    int m_deviceIdToDeviceTabIdx[Device::max];      //!< Map from DeviceId to m_deviceList index.
    int m_suicideFd;                                //!< Activity on this file descriptor kills the process.
    int fd(int deviceId) const;
    int cardNameToNum(const char *target) const;
	int openRaw(const char *portName, int mode) const;
    void openDevices();
public:
	Driver(WINDOW *window);
    int wait(int usecTimeout = 0, int device = Device::all) const;
	uint8_t getByte(int device);
	void putByte(int device, uint8_t b1);
    void putBytes(int device, const uint8_t *b, int n);
	void putBytes(int device, uint8_t b1, uint8_t b2);
	void putBytes(int device, uint8_t b1, uint8_t b2, uint8_t b3);
};

const char *noteName(uint8_t num);
void noteName(uint8_t num, char *s);
bool isNote(uint8_t status);
bool isNoteOn(uint8_t status, uint8_t data1, uint8_t data2);
bool isNoteOff(uint8_t status, uint8_t data1, uint8_t data2);
bool isController(uint8_t status);

} // namespace Midi

#endif // MIDI_H
