/*! \file mididriver.cpp
 *  \brief Contains objects that handle all MIDI traffic.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "error.h"
#include "mididriver.h"
#include "timer.h"
#include "patchercore.h"

namespace Midi {

/*! \brief My setup.
 */
Device deviceList[] =
{
    { "Anniv",   0, 0, 0, in,  Device::A30,       "A30", "Roland A30",        0 },
    { "Anniv",   0, 0, 0, out, Device::FantomOut, "Fan", "Roland Fantom XR",  0 },
    { "Anniv",   0, 0, 1, in,  Device::Fcb1010,   "FCB", "Behringer FCB1010", 0 },
    { "Anniv",   0, 0, 1, out, Device::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 2, in,  Device::FantomIn,  "Fan", "Roland Fantom XR",  0 },
    { "Anniv",   0, 0, 2, out, Device::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 3, in,  Device::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 3, out, Device::none,      "---", "-",                 0 },
    { "BCF2000", 0, 0, 0, in,  Device::BcfIn,     "BCF", "Behringer BCF2000", 0 },
    { "BCF2000", 0, 0, 0, out, Device::BcfOut,    "BCF", "Behringer BCF2000", 0 },
    { 0,         0, 0, 0, none,Device::none,      0,     0,                   0 },
};


/*! \brief Map from DevId to file descriptor.
 *
 *  \param[in]  deviceId    A valid \a DevId.
 */
int Driver::fd(int deviceId) const
{
    if (deviceId > Device::none && deviceId < Device::max)
        return m_deviceList[m_deviceIdToDeviceTabIdx[deviceId]].m_fd;
    return -1;
}

/*! \brief Obtain file descriptors for all devices.
 */
void Driver::openDevices()
{
    Device *dIn = m_deviceList;
    char devStr[100];
    int deviceIdx = 0;
    while (dIn->m_longDescription != 0)
    {
        Device *dOut = dIn+1;
        dOut->m_card = dIn->m_card = cardNameToNum(dIn->m_cardName);
        sprintf(devStr, "hw:%d,%d,%d", dIn->m_card, dIn->m_sub1, dIn->m_sub2);
        if (dIn->m_id != Device::none && dOut->m_id != Device::none)
        {
            if (m_window)
            {
                wprintw(m_window, "opening %s\n%s\n", dIn->m_longDescription, devStr);
                wprintw(m_window, "opening %s\n%s\n", dOut->m_longDescription, devStr);
                wrefresh(m_window);
            }
            dIn->m_fd = dOut->m_fd = openRaw(devStr, O_RDWR);
        }
        else if (dIn->m_id == Device::none && dOut->m_id != Device::none)
        {
            if (m_window)
            {
                wprintw(m_window, "opening %s\n%s\n", dOut->m_longDescription, devStr);
                wrefresh(m_window);
            }
            dOut->m_fd = openRaw(devStr, O_WRONLY);
        }
        else if (dIn->m_id != Device::none && dOut->m_id == Device::none)
        {
            if (m_window)
            {
                wprintw(m_window, "opening %s\n%s\n", dIn->m_longDescription, devStr);
                wrefresh(m_window);
            }
            dIn->m_fd = openRaw(devStr, O_RDONLY);
        }
        else
        {
            // nothing
        }
        m_deviceIdToDeviceTabIdx[dIn->m_id] = deviceIdx;
        m_deviceIdToDeviceTabIdx[dOut->m_id] = deviceIdx+1;
        dIn += 2;
        deviceIdx += 2;
    }
    if (m_window)
    {
        for (int i=Device::none+1; i<Device::max; i++)
        {
            wprintw(m_window, "dev = %d, devIdx = %d, fd = %d\n",
                i, m_deviceIdToDeviceTabIdx[i], fd(i));
        }
        wrefresh(m_window);
    }
}

/*! \brief Open a raw midi device.
 *
 * \param[in] portName  ALSA port name.
 * \param[in] mode      open(2) flag.
 * \return file descriptor, or -1 on error.
 */
int Driver::openRaw(const char *portName, int mode) const
{
   // abandoned once we have the file descriptor
   snd_rawmidi_t *rawMidi1, *rawMidi2;
   int status;
   if ((status =
        snd_rawmidi_open(&rawMidi1, &rawMidi2, portName, 0)) < 0) {
        if (m_window)
            wprintw(m_window, "cannot open MIDI port %s: %s",
                portName, snd_strerror(status));
    return -1;
   }
    struct pollfd pfds;
    if (mode & O_WRONLY)
        snd_rawmidi_poll_descriptors(rawMidi2, &pfds, 1);
    else
        snd_rawmidi_poll_descriptors(rawMidi1, &pfds, 1);
    return pfds.fd;
}

/*! \brief wait for activity on one or all devices.
 *
 * If there is any activity on the suicide-fd then this function throws an exception
 * which will presumably kill this process.
 *
 * \param[in] usecTimeout    timeout in usec, if specified.
 * \param[in] device         specified device ID to wait for, if specified, otherwise any activity.
 */
int Driver::wait(int usecTimeout, int device) const
{
    fd_set fdSet;
    FD_ZERO(&fdSet);
    int maxFd = 0;
    if (device == Device::all)
    {
        for (int i=0; m_deviceList[i].m_longDescription;i++)
        {
            if (m_deviceList[i].m_direction == in && m_deviceList[i].m_id != Device::none)
            {
                FD_SET(m_deviceList[i].m_fd, &fdSet);
                if (m_deviceList[i].m_fd > maxFd)
                    maxFd = m_deviceList[i].m_fd;
            }
        }
        //FD_SET(0, &fdSet); // add stdin
    }
    else
    {
        maxFd = fd(device);
        FD_SET(maxFd, &fdSet);
    }
    FD_SET(m_suicideFd, &fdSet);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usecTimeout;
    if (m_window)
        wrefresh(m_window);
    int e;
    bool interrupted = false;
    do
    {
        e = select(maxFd+1, &fdSet, NULL, NULL, usecTimeout> 0 ? &tv : NULL);
        interrupted = e == -1 && errno == EINTR;
    } while (interrupted && !g_timer.timedout());
    if (e == -1)
    {
        throw(Error("select", errno));
    }
    for (int i=0; m_deviceList[i].m_longDescription;i++)
    {
        if (m_deviceList[i].m_id != Device::none &&
            m_deviceList[i].m_direction == in &&
            FD_ISSET(m_deviceList[i].m_fd, &fdSet))
        {
#if 0
            wprintw(m_window,
                    "fd %d is ready, idx = %d, dev = %d\n",
                    m_deviceList[i].m_fd, i, m_deviceList[i].m_id);
#endif
            return m_deviceList[i].m_id;
        }
    }
    throw(Error("non-midi activity", errno));
    return -1;
}

/*! \brief Map an ALSA card name to a card number.
 *
 * \param[in] target        Card name to look for.
 * \return                  The card number.
 */
int Driver::cardNameToNum(const char *target) const
{
    const char *proc = "/proc/asound/cards";
    int hwNum = -1;
    FILE *fp = fopen(proc, "r");
    if (g_timer.timedout() || !fp)
    {
        throw(Error("open", errno));
    }
    char procLineBuf[100];
    for (;;)
    {
        if (!fgets(procLineBuf, sizeof(procLineBuf), fp))
            break;
        if (sscanf(procLineBuf, "%d", &hwNum) == 1)
        {
            if (strstr(procLineBuf, target) != 0)
                break;
        }
        hwNum = -1;
    }
    fclose(fp);
    if (hwNum == -1 && m_window)
    {
        wprintw(m_window, "cannot find %s\n", target);
    }
    return hwNum;
}

/*! \brief Construct a MIDI \a Driver object
 *
 * \param[in] win     A curses WINDOW object to log to.
 */
Driver::Driver(WINDOW *win): m_window(win)
{
    m_deviceList = &deviceList[0];
    openDevices();
    m_suicideFd = inotify_init();
    int wd = inotify_add_watch(m_suicideFd, TRACK_DEF, IN_MODIFY);
    if (wd < 0)
    {
        throw(Error("inotify_add_watch", errno));
    }
}

/*! \brief Get a byte from a MIDI device.
 *
 * This function blocks, and an \a Alarm may cause an exception.
 *
 * \param[in] device    Device ID.
 * \return              MIDI byte.
 */
uint8_t Driver::getByte(int device) const
{
    if (device < 0)
        return 0;
    uint8_t buf;
    g_timer.read(fd(device), &buf, 1);
    return buf;
}

/*! \brief Write a byte to a MIDI device.
 *
 * \param[in]   device  MIDI device.
 * \param[in]   b1      byte to be written.
 */
void Driver::putByte(int device, uint8_t b1) const
{
    int fDescr = fd(device);
    if (fDescr >= 0)
        g_timer.write(fDescr, &b1, 1);
}

/*! \brief Write a byte string to a MIDI device.
 *
 * \param[in]   device  MIDI device.
 * \param[in]   b       byte buffer.
 * \param[in]   n       byte buffer size.
 */
void Driver::putBytes(int device, const uint8_t *b, int n) const
{
    int fDescr = fd(device);
    if (fDescr >= 0)
        g_timer.write(fDescr, b, n);
}

/*! \brief Write 2 bytes to a MIDI device.
 *
 * \param[in]   device  MIDI device.
 * \param[in]   b1      first byte.
 * \param[in]   b2      second byte.
 */
void Driver::putBytes(int device, uint8_t b1, uint8_t b2) const
{
    int fDescr = fd(device);
    if (fDescr >= 0)
    {
        uint8_t buf[2];
        buf[0] = b1;
        buf[1] = b2;
        g_timer.write(fDescr, buf, 2);
    }
}

/*! \brief Write 3 bytes to a MIDI device.
 *
 * \param[in]   device  MIDI device.
 * \param[in]   b1      first byte.
 * \param[in]   b2      second byte.
 * \param[in]   b3      third byte.
 */
void Driver::putBytes(int device, uint8_t b1, uint8_t b2, uint8_t b3) const
{
    int fDescr = fd(device);
    if (fDescr >= 0)
    {
        uint8_t buf[3];
        buf[0] = b1;
        buf[1] = b2;
        buf[2] = b3;
        g_timer.write(fDescr, buf, 3);
    }
}

} // namespace Midi
