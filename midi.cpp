#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "error.h"
#include "midi.h"

extern int globalTimeout;

namespace {

enum { in = 100, out };

MidiDevice deviceList[] = 
{
    { "Anniv",   0, 0, 0, in,  MidiDevice::A30,       "A30", "Roland A30",        0 },
    { "Anniv",   0, 0, 0, out, MidiDevice::FantomOut, "Fan", "Roland Fantom XR",  0 },
    { "Anniv",   0, 0, 1, in,  MidiDevice::Fcb1010,   "FCB", "Behringer FCB1010", 0 },
    { "Anniv",   0, 0, 1, out, MidiDevice::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 2, in,  MidiDevice::FantomIn,  "Fan", "Roland Fantom XR",  0 },
    { "Anniv",   0, 0, 2, out, MidiDevice::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 3, in,  MidiDevice::none,      "---", "-",                 0 },
    { "Anniv",   0, 0, 3, out, MidiDevice::none,      "---", "-",                 0 },
    { "BCF2000", 0, 0, 0, in,  MidiDevice::BcfIn,     "BCF", "Behringer BCF2000", 0 },
    { "BCF2000", 0, 0, 0, out, MidiDevice::BcfOut,    "BCF", "Behringer BCF2000", 0 },
    { 0,         0, 0, 0, 0,   0,                     0,     0,                   0 },
};

};

int Midi::fd(int deviceId) const
{
    if (deviceId > MidiDevice::none && deviceId < MidiDevice::max)
        return m_deviceList[m_deviceIdToDeviceTabIdx[deviceId]].m_fd;
    return -1;
}

void Midi::openDevices(void)
{
    MidiDevice *dIn = m_deviceList;
    char devStr[100];
    int deviceIdx = 0;
    while (dIn->m_longDescr != 0)
    {
        MidiDevice *dOut = dIn+1;
        dOut->m_card = dIn->m_card = cardNameToNum(dIn->m_cardName);
        sprintf(devStr, "hw:%d,%d,%d", dIn->m_card, dIn->m_sub1, dIn->m_sub2);
        if (dIn->m_id != MidiDevice::none && dOut->m_id != MidiDevice::none)
        {
            m_screen->printMidi("opening %s\n%s\n", dIn->m_longDescr, devStr);
            m_screen->printMidi("opening %s\n%s\n", dOut->m_longDescr, devStr);
            m_screen->flushMidi();
            dIn->m_fd = dOut->m_fd = openRaw(devStr, O_RDWR);
        }
        else if (dIn->m_id == MidiDevice::none && dOut->m_id != MidiDevice::none)
        {
            m_screen->printMidi("opening %s\n%s\n", dOut->m_longDescr, devStr);
            m_screen->flushMidi();
            dOut->m_fd = openRaw(devStr, O_WRONLY);
        }
        else if (dIn->m_id != MidiDevice::none && dOut->m_id == MidiDevice::none)
        {
            m_screen->printMidi("opening %s\n%s\n", dIn->m_longDescr, devStr);
            m_screen->flushMidi();
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
    for (int i=MidiDevice::none+1; i<MidiDevice::max; i++)
    {
        m_screen->printMidi("dev = %d, devIdx = %d, fd = %d\n", 
            i, m_deviceIdToDeviceTabIdx[i], fd(i));
    }
    m_screen->flushMidi();
}

void Midi::noteName(uint8_t num, char *s)
{
    const char *str[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    int oct = num / 12;
    num = num % 12;
    sprintf(s, "%s%d", str[num], oct);
}

const char *Midi::noteName(uint8_t num)
{
    static char buf[5];
    noteName(num, buf);
    return buf;
}

int Midi::openRaw(const char *portName, int mode) const
{
   // abandoned once we have the file descriptor
   snd_rawmidi_t *rawMidi1, *rawMidi2; 
   int status;
   if ((status = 
        snd_rawmidi_open(&rawMidi1, &rawMidi2, portName, 0)) < 0) {
        m_screen->printMidi("cannot open MIDI port %s: %s", 
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

int Midi::wait(int usecTimeout, int device) const
{
    fd_set fdSet;
    FD_ZERO(&fdSet);
    int maxFd = 0;
    if (device == MidiDevice::all)
    {
        for (int i=0; m_deviceList[i].m_longDescr;i++)
        {
            if (m_deviceList[i].m_direction == in && m_deviceList[i].m_id != MidiDevice::none)
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
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usecTimeout;
    m_screen->flushMidi();
    int e;
    bool interrupted = false;
    do 
    {
        e = select(maxFd+1, &fdSet, NULL, NULL, usecTimeout> 0 ? &tv : NULL);
        interrupted = e == -1 && errno == EINTR;
    } while (interrupted && !globalTimeout);
    if (e == -1)
    {
        throw(Error("select", errno));
    }
    for (int i=0; m_deviceList[i].m_longDescr;i++)
    {
        if (m_deviceList[i].m_id != MidiDevice::none && 
            m_deviceList[i].m_direction == in && 
            FD_ISSET(m_deviceList[i].m_fd, &fdSet))
        {
#if 0
            m_screen->printMidi(
                    "fd %d is ready, idx = %d, dev = %d\n",
                    m_deviceList[i].m_fd, i, m_deviceList[i].m_id);
#endif
            return m_deviceList[i].m_id;
        }
    }
    return -1;
}

int Midi::cardNameToNum(const char *target) const
{
    const char *proc = "/proc/asound/cards";
    int hwNum = -1;
    FILE *fp = fopen(proc, "r");
    if (globalTimeout || !fp)
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
    if (hwNum == -1)
    {
        m_screen->printMidi("cannot find %s\n", target);
    }
    return hwNum;
}

Midi::Midi(Screen *s): m_screen(s)
{
    m_deviceList = &deviceList[0];
    openDevices();
}

uint8_t Midi::getByte(int device)
{
    if (device < 0)
        return 0;
    uint8_t buf;
    for (;;)
    {
        ssize_t n = read(fd(device), &buf, 1);
        if (n >= 0)
            break;
        if (globalTimeout || (n < 0 && (errno != EAGAIN && errno != EINTR)))
        {
            throw(Error("read", errno));
        }
        perror("read");
    }
    return buf;
}

static void writeChecked(int fd, const void *buf, size_t count)
{
    for (;;)
    {
        ssize_t rv = write(fd, buf, count);
        if (globalTimeout || (rv == -1 && errno != EINTR))
        {
            throw(Error("write", errno));
        }
        if (rv == (ssize_t)count)
        {
            break;
        }
        else
        {
            count -= rv;
            buf = (char*)buf + count;
        }
    }
}

void Midi::putByte(int device, uint8_t b1)
{
    int fDescr = fd(device);
    if (fDescr >= 0)
        writeChecked(fDescr, &b1, 1);
}

void Midi::putBytes(int device, const uint8_t *b, int n)
{
    int fDescr = fd(device);
    if (fDescr >= 0)
        writeChecked(fDescr, b, n);
}

void Midi::putBytes(int device, uint8_t b1, uint8_t b2)
{
    int fDescr = fd(device);
    if (fDescr >= 0)
    {
        uint8_t buf[2];
        buf[0] = b1;
        buf[1] = b2;
        writeChecked(fDescr, buf, 2);
    }
}

void Midi::putBytes(int device, uint8_t b1, uint8_t b2, uint8_t b3)
{
    int fDescr = fd(device);
    if (fDescr >= 0)
    {
        uint8_t buf[3];
        buf[0] = b1;
        buf[1] = b2;
        buf[2] = b3;
        writeChecked(fDescr, buf, 3);
    }
}

bool Midi::isNote(uint8_t status)
{
    status &= 0xf0;
    return status == MidiStatus::noteOff 
        || status == MidiStatus::noteOn 
        || status == MidiStatus::aftertouch;
}

bool Midi::isNoteOn(uint8_t status, uint8_t data1, uint8_t data2)
{
    (void)data1;
    status &= 0xf0;
    return status == MidiStatus::noteOn && data2 != 0;
}

bool Midi::isNoteOff(uint8_t status, uint8_t data1, uint8_t data2)
{
    (void)data1;
    status &= 0xf0;
    return (status == MidiStatus::noteOn && data2 == 0) 
         || status == MidiStatus::noteOff;
}

bool Midi::isController(uint8_t status)
{
    status &= 0xf0;
    return status == MidiStatus::controller;
}

Midi::~Midi(void)
{
}

