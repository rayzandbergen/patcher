#include "fantom.h"
#include "error.h"

void FantomPart::constructPreset(bool &patchReadAllowed)
{
    patchReadAllowed = false;
    if (m_bankSelectMsb == 87)
    {
        patchReadAllowed = true;
        switch (m_bankSelectLsb)
        {
            case 0:
            case 1:
                sprintf(m_preset, "USR %03d", m_bankSelectLsb*128+m_pCh+1);
                break;
            case 32:
            case 33:
                sprintf(m_preset, "CRD %03d", (m_bankSelectLsb-32)*128+m_pCh+1);
                break;
            default:
                sprintf(m_preset, "PR%c %03d", (m_bankSelectLsb-64+'A'), m_pCh+1);
                break;
        }
    }
    else if (m_bankSelectMsb == 86)
    {
        switch (m_bankSelectLsb)
        {
            case 0:
                sprintf(m_preset, "USRr%03d", m_bankSelectLsb*128+m_pCh+1);
                break;
            case 32:
                sprintf(m_preset, "CRDr%03d", (m_bankSelectLsb-32)*128+m_pCh+1);
                break;
            default:
                sprintf(m_preset, "PRr%c%03d", (m_bankSelectLsb-64+'A'), m_pCh+1);
                break;
        }
    }
    else if (m_bankSelectMsb == 120)
    {
        sprintf(m_preset, " GMr%03d", m_bankSelectLsb*128+m_pCh+1);
    }
    else if (m_bankSelectMsb == 121)
    {
        sprintf(m_preset, " GM %03d", m_bankSelectLsb*128+m_pCh+1);
    }
    else
    {
        sprintf(m_preset, "?%d-%d-%d?", m_bankSelectMsb, m_bankSelectLsb, m_pCh);
    }
}

void FantomPart::save(const Dump *d) 
{
    d->save(m_channel);
    d->save(m_bankSelectMsb);
    d->save(m_bankSelectLsb);
    d->save(m_pCh);
    d->save(m_vol);
    d->save(m_transpose);
    d->save(m_oct);
    d->save((const char*)m_preset);
    d->save(m_keyRangeLower);
    d->save(m_keyRangeUpper);
    d->save(m_fadeWidthLower);
    d->save(m_fadeWidthUpper);
    m_patch.save(d);
}

void FantomPart::restore(const Dump *d)
{
    d->restore(m_channel);
    d->restore(m_bankSelectMsb);
    d->restore(m_bankSelectLsb);
    d->restore(m_pCh);
    d->restore(m_vol);
    d->restore(m_transpose);
    d->restore(m_oct);
    d->restore((char*)m_preset);
    d->restore(m_keyRangeLower);
    d->restore(m_keyRangeUpper);
    d->restore(m_fadeWidthLower);
    d->restore(m_fadeWidthUpper);
    m_patch.restore(d);
}

void FantomPart::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.preset:%s\n", prefix, m_preset);
    screen->printLog("%s.name:%s\n", prefix, m_patch.m_name);
    screen->printLog("%s.channel:%d\n", prefix, m_channel);
    screen->printLog("%s.vol:%d\n", prefix, m_vol);
    screen->printLog("%s.transpose:%d\n", prefix, m_transpose);
    screen->printLog("%s.oct:%d\n", prefix, m_oct);
    screen->printLog("%s.keyRangeLower:%s\n", prefix, Midi::noteName(m_keyRangeLower));
    screen->printLog("%s.keyRangeUpper:%s\n", prefix, Midi::noteName(m_keyRangeUpper));
    screen->printLog("%s.fadeWidthLower:%d\n", prefix, m_fadeWidthLower);
    screen->printLog("%s.fadeWidthUpper:%d\n", prefix, m_fadeWidthUpper);
}

void FantomPerformance::save(const Dump *d) 
{
    d->save((const char*)m_name);
    for (int i=0; i<16; i++)
        m_part[i].save(d);
}

void FantomPerformance::restore(const Dump *d) 
{
    d->restore((char*)m_name);
    for (int i=0; i<16; i++)
        m_part[i].restore(d);
}

void Fantom::setParam(const uint32_t addr, const uint32_t length, uint8_t *data)
{
    //m_screen->printLog("Fantom::setParam %08x %08x\n", addr, length);
    uint8_t txBuf[128];
    uint32_t checkSum = 0;
    uint32_t i=0;
    txBuf[i++] = MidiStatus::sysEx;
    txBuf[i++] = 0x41; // ID: Roland
    uint32_t checkSumStart = i;
    txBuf[i++] = 0x10; // dev ID, start of checksum
    txBuf[i++] = 0x00; // model fantom XR
    txBuf[i++] = 0x6b; // model fantom XR
    txBuf[i++] = 0x12; // command ID
    txBuf[i++] = (uint8_t)(0xff & addr >> 24);
    txBuf[i++] = (uint8_t)(0xff & addr >> 16);
    txBuf[i++] = (uint8_t)(0xff & addr >> 8);
    txBuf[i++] = (uint8_t)(0xff & addr);
    if (i + length + 2 > sizeof(txBuf))
        throw(Error("Fantom::setParam: txBuf overflow"));
    for (uint32_t j=0; j<length; j++)
        txBuf[i++] = data[j];
    uint32_t checkSumEnd = i;
    checkSum = 0;
    for (uint32_t j=checkSumStart; j<=checkSumEnd; j++)
        checkSum += txBuf[j];
    txBuf[i++] = 0x80 - (checkSum & 0x7f);
    txBuf[i++] = MidiStatus::EOX;
    m_midi->putBytes(MidiDevice::FantomOut, txBuf, i);
#if 0
    m_screen->printMidi("sending sysEx %08x %08x\n", addr, length);
    m_screen->dumpToLog(txBuf, i);
    m_screen->flushMidi();
#endif
}

void Fantom::getParam(const uint32_t addr, const uint32_t length, uint8_t *data)
{
    const uint32_t rxHeaderLength = 10;
    //m_screen->printLog("Fantom::getParam %08x %08x\n", addr, length);
    uint8_t txBuf[128];
    uint8_t rxBuf[128]; // debug only
    memset(rxBuf, 0, sizeof(rxBuf));
    uint32_t checkSum = 0;
    uint32_t i=0;
    txBuf[i++] = MidiStatus::sysEx;
    txBuf[i++] = 0x41; // ID: Roland
    uint32_t checkSumStart = i;
    txBuf[i++] = 0x10; // dev ID, start of checksum
    txBuf[i++] = 0x00; // model fantom XR
    txBuf[i++] = 0x6b; // model fantom XR
    txBuf[i++] = 0x11; // command ID = RQ1
    txBuf[i++] = (uint8_t)(0xff & addr >> 24);
    txBuf[i++] = (uint8_t)(0xff & addr >> 16);
    txBuf[i++] = (uint8_t)(0xff & addr >> 8);
    txBuf[i++] = (uint8_t)(0xff & addr);
    txBuf[i++] = (uint8_t)(0xff & length >> 24);
    txBuf[i++] = (uint8_t)(0xff & length >> 16);
    txBuf[i++] = (uint8_t)(0xff & length >> 8);
    uint32_t checkSumEnd = i;
    txBuf[i++] = (uint8_t)(0xff & length);
    checkSum = 0;
    if (i + length + 2 > sizeof(txBuf))
        throw(Error("Fantom::getParam: txBuf overflow"));
    for (uint32_t j=checkSumStart; j<=checkSumEnd; j++)
        checkSum += txBuf[j];
    txBuf[i++] = 0x80 - (checkSum & 0x7f);
    txBuf[i++] = MidiStatus::EOX;
    m_midi->putBytes(MidiDevice::FantomOut, txBuf, i);
    uint8_t byteRx;
    for (;;)
    {
        byteRx = m_midi->getByte(MidiDevice::FantomIn);
        if (byteRx == MidiStatus::sysEx)
            break;
        m_screen->printMidi("unexpected response, retry\n");
        m_screen->flushMidi();
    }
    rxBuf[0] = byteRx;
    for (i=1;;i++)
    {
        byteRx = m_midi->getByte(MidiDevice::FantomIn);
        rxBuf[i] = byteRx;
        if (byteRx == MidiStatus::EOX || i+1 >= (uint32_t)sizeof(rxBuf))
            break;
        if (i >= rxHeaderLength && i-rxHeaderLength < length)
        {
            data[i-rxHeaderLength] = byteRx;
        }
        // TODO maybe Rx header check, checksum check
    }
    //m_screen->printLog("received, length = %d\n", i);
    //m_screen->dumpToLog(rxBuf, i);
}

void Fantom::setVolume(uint8_t part, uint8_t val)
{
    uint32_t addr = 0x10000000 + ((0x20+part)<<8) + 7;
    setParam(addr, 1, &val);
}

void Fantom::getPartParams(FantomPart *p, int idx)
{
    const int partSize = 0x31;
    uint8_t buf[partSize];
    getParam(0x10000000 + ((0x20+idx)<<8), partSize, buf);
    p->m_channel = buf[0];
    p->m_bankSelectMsb = buf[4];
    p->m_bankSelectLsb = buf[5];
    p->m_pCh = buf[6];
    p->m_vol = buf[0x07];
    p->m_transpose = buf[0x09] - 64;
//    m_screen->printLog("transpose = %d\n", p->m_transpose);
    p->m_oct = buf[0x15] - 64;
    p->m_keyRangeLower = buf[0x17];
    p->m_keyRangeUpper = buf[0x18];
    p->m_fadeWidthLower = buf[0x19];
    p->m_fadeWidthUpper = buf[0x1a];
}

void Fantom::getPatchName(char *s, int idx)
{
    uint32_t offset = 0x20 * idx;
    uint32_t offsetLo = offset & 0x7f;
    uint32_t offsetHi = offset >> 7;
    uint32_t addr = 0x11000000 + (offsetHi << 24) + (offsetLo << 16);
    uint8_t buf[nameSize];
    memset(buf, '*', sizeof(buf));
    getParam(addr, nameSize, buf);
    memcpy(s, buf, nameSize);
    s[nameSize] = 0;
}

void Fantom::getPerfName(char *s)
{
    uint8_t buf[nameSize];
    memset(buf, '*', sizeof(buf));
    getParam(0x10000000, nameSize, buf);
    memcpy(s, buf, nameSize);
    s[nameSize] = 0;
}

void Fantom::setPerfName(const char *s)
{
    uint8_t buf[nameSize];
    memset(buf, ' ', sizeof(buf));
    for (int i=0; s[i] && i<nameSize; i++)
    {
        buf[i] = s[i];
    }
    setParam(0x10000000, nameSize, buf);
}

void Fantom::setPartName(int part, const char *s)
{
    uint8_t buf[nameSize];
    memset(buf, ' ', sizeof(buf));
    for (int i=0; s[i] && i<nameSize; i++)
    {
        buf[i] = s[i];
    }
    uint32_t addr = 0x11000000 + 0x200000*(uint32_t)part;
    setParam(addr, nameSize, buf);
}

