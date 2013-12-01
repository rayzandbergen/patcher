/*! \file mididriver.h
 *  \brief Contains objects that handle all MIDI traffic.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H
#include <stdint.h>
#include "screen.h"

//! \brief Namespace for all MIDI driver objects.
namespace Midi
{

#ifndef NO_MIDI_DRIVER
//! \brief Direction of MIDI data, as seen from Pi.
enum InOut { none, in = 100, out };

//! \brief State of a single MIDI device.
struct Device
{
    //! \brief My hardware.
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
 * This object logs to a curses WINDOW object.
 */
class Driver {
    WINDOW *m_window;                               //!< a curses WINDOW object to log to.
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
#endif // NO_MIDI_DRIVER

} // namespace Midi

#endif // MIDI_DRIVER_H
