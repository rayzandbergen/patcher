/*! \file fantom.h
 *  \brief Contains classes to store patches/parts/performances (Roland-speak) from the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "fantom.h"
#include "error.h"

namespace Fantom
{

/*! \brief Construct the preset name for \a this.
 *
 *  \param[in] patchReadAllowed  True if the patch may be read. Fantom disallows download of some GM sounds.
 */
void Part::constructPreset(bool &patchReadAllowed)
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

/*! \brief Save \a this to a \a Dump object.
 *
 * \param[in] d object to dump to.
 */
void Part::save(const Dump *d)
{
    d->save(m_number);
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

/*! \brief Restore \a this from a \a Dump object.
 *
 * \param[in] d object to read from.
 */
void Part::restore(const Dump *d)
{
    d->restore(m_number);
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

/*! \brief Dump \a this to \a Screen.
 *
 * \param[in] screen Screen object to dump to
 * \param[in] prefix Print prefix.
 */
void Part::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.preset:%s\n", prefix, m_preset);
    screen->printLog("%s.name:%s\n", prefix, m_patch.m_name);
    screen->printLog("%s.channel:%d\n", prefix, 1+m_channel);
    screen->printLog("%s.vol:%d\n", prefix, m_vol);
    screen->printLog("%s.transpose:%d\n", prefix, m_transpose);
    screen->printLog("%s.oct:%d\n", prefix, m_oct);
    screen->printLog("%s.keyRangeLower:%s\n", prefix, Midi::noteName(m_keyRangeLower));
    screen->printLog("%s.keyRangeUpper:%s\n", prefix, Midi::noteName(m_keyRangeUpper));
    screen->printLog("%s.fadeWidthLower:%d\n", prefix, m_fadeWidthLower);
    screen->printLog("%s.fadeWidthUpper:%d\n", prefix, m_fadeWidthUpper);
}

/*! \brief Save \a this to a \a Dump object.
 *
 * \param[in] d object to dump to.
 */
void Performance::save(const Dump *d)
{
    d->save((const char*)m_name);
    for (int i=0; i<NofParts; i++)
        m_part[i].save(d);
}

/*! \brief Restore \a this from a \a Dump object.
 *
 * \param[in] d object to read from.
 */
void Performance::restore(const Dump *d)
{
    d->restore((char*)m_name);
    for (int i=0; i<NofParts; i++)
        m_part[i].restore(d);
}

/*! \brief Set a parameter in Fantom memory.
 *
 * \param[in] addr      Parameter address.
 * \param[in] length    Number of bytes to be written.
 * \param[in] data      Byte string.
 */
void Driver::setParam(const uint32_t addr, const uint32_t length, uint8_t *data)
{
    //m_screen->printLog("Driver::setParam %08x %08x\n", addr, length);
    uint8_t txBuf[128];
    uint32_t checkSum = 0;
    uint32_t i=0;
    txBuf[i++] = Midi::sysEx;
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
        throw(Error("Driver::setParam: txBuf overflow"));
    for (uint32_t j=0; j<length; j++)
        txBuf[i++] = data[j];
    uint32_t checkSumEnd = i;
    checkSum = 0;
    for (uint32_t j=checkSumStart; j<=checkSumEnd; j++)
        checkSum += txBuf[j];
    txBuf[i++] = 0x80 - (checkSum & 0x7f);
    txBuf[i++] = Midi::EOX;
    m_midi->putBytes(Midi::Device::FantomOut, txBuf, i);
#if 0
    m_screen->printMidi("sending sysEx %08x %08x\n", addr, length);
    m_screen->dumpToLog(txBuf, i);
    m_screen->flushMidi();
#endif
}

/*! \brief Retrieve a parameter fom Fantom memory.
 *
 * \param[in] addr      Parameter address.
 * \param[in] length    Number of bytes to get.
 * \param[out] data     Byte string.
 */
void Driver::getParam(const uint32_t addr, const uint32_t length, uint8_t *data)
{
    const uint32_t rxHeaderLength = 10;
    //m_screen->printLog("Driver::getParam %08x %08x\n", addr, length);
    uint8_t txBuf[128];
    uint8_t rxBuf[128]; // debug only
    memset(rxBuf, 0, sizeof(rxBuf));
    uint32_t checkSum = 0;
    uint32_t i=0;
    txBuf[i++] = Midi::sysEx;
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
        throw(Error("Driver::getParam: txBuf overflow"));
    for (uint32_t j=checkSumStart; j<=checkSumEnd; j++)
        checkSum += txBuf[j];
    txBuf[i++] = 0x80 - (checkSum & 0x7f);
    txBuf[i++] = Midi::EOX;
    m_midi->putBytes(Midi::Device::FantomOut, txBuf, i);
    uint8_t byteRx;
    for (;;)
    {
        byteRx = m_midi->getByte(Midi::Device::FantomIn);
        if (byteRx == Midi::sysEx)
            break;
        m_screen->printMidi("unexpected response, retry\n");
        m_screen->flushMidi();
    }
    rxBuf[0] = byteRx;
    for (i=1;;i++)
    {
        byteRx = m_midi->getByte(Midi::Device::FantomIn);
        rxBuf[i] = byteRx;
        if (byteRx == Midi::EOX || i+1 >= (uint32_t)sizeof(rxBuf))
            break;
        if (i >= rxHeaderLength && i-rxHeaderLength < length)
        {
            data[i-rxHeaderLength] = byteRx;
        }
        // \todo check Rx header check and checksum
    }
    //m_screen->printLog("received, length = %d\n", i);
    //m_screen->dumpToLog(rxBuf, i);
}

/*! \brief Set the volume of a part.
 *
 * \param[in] part      part number.
 * \param[in] val       Volume value.
 */
void Driver::setVolume(uint8_t part, uint8_t val)
{
    uint32_t addr = 0x10000000 + ((0x20+part)<<8) + 7;
    setParam(addr, 1, &val);
}

/*! \brief Retrieve all the relevant parameters of a Fantom Part.
 *
 * \param[in] p         Pointer to a Part.
 * \param[in] idx       Part index within Performance.
 */
void Driver::getPartParams(Part *p, int idx)
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

/*! \brief Retrieve a patch name from a Fantom Performance.
 *
 * \param[in] s         Pointer to char buffer of at least NameLength+1 chars.
 * \param[in] idx       Part index within Performance.
 */
void Driver::getPatchName(char *s, int idx)
{
    uint32_t offset = 0x20 * idx;
    uint32_t offsetLo = offset & 0x7f;
    uint32_t offsetHi = offset >> 7;
    uint32_t addr = 0x11000000 + (offsetHi << 24) + (offsetLo << 16);
    uint8_t buf[NameLength+1];
    memset(buf, '*', sizeof(buf));
    getParam(addr, NameLength, buf);
    memcpy(s, buf, NameLength);
    s[NameLength] = 0;
}

/*! \brief Retrieve a Performance name from the Fantom.
 *
 * \param[in] s         Pointer to char buffer of at least NameLength+1 chars.
 */
void Driver::getPerfName(char *s)
{
    uint8_t buf[NameLength+1];
    memset(buf, '*', sizeof(buf));
    getParam(0x10000000, NameLength, buf);
    memcpy(s, buf, NameLength);
    s[NameLength] = 0;
}

/*! \brief Set a Performance name in the Fantom.
 *
 * \param[in] s         Pointer to char buffer, optionally zero-terminated.
 */
void Driver::setPerfName(const char *s)
{
    uint8_t buf[NameLength+1];
    memset(buf, ' ', sizeof(buf));
    for (int i=0; s[i] && i<NameLength; i++)
    {
        buf[i] = s[i];
    }
    setParam(0x10000000, NameLength, buf);
}

/*! \brief Set a Part name in the Fantom.
 *
 * \param[in] part  Index of the part within the performance.
 * \param[in] s     Pointer to char buffer, optionally zero-terminated.
 */
void Driver::setPartName(int part, const char *s)
{
    uint8_t buf[NameLength+1];
    memset(buf, ' ', sizeof(buf));
    for (int i=0; s[i] && i<NameLength; i++)
    {
        buf[i] = s[i];
    }
    uint32_t addr = 0x11000000 + 0x200000*(uint32_t)part;
    setParam(addr, NameLength, buf);
}

} // namespace Fantom

