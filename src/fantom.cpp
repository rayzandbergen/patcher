/*! \file fantom.h
 *  \brief Contains classes to store patches/parts/performances (Roland-speak) from the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "fantom.h"
#include "error.h"
#include "timestamp.h"
#include "screen.h"
#include "mididef.h"

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
    d->saveInt(m_number);
    d->saveInt(m_channel);
    d->saveInt(m_bankSelectMsb);
    d->saveInt(m_bankSelectLsb);
    d->saveInt(m_pCh);
    d->saveInt(m_vol);
    d->saveInt(m_transpose);
    d->saveInt(m_oct);
    d->save((const char*)m_preset);
    d->saveInt(m_keyRangeLower);
    d->saveInt(m_keyRangeUpper);
    d->saveInt(m_fadeWidthLower);
    d->saveInt(m_fadeWidthUpper);
    m_patch.save(d);
}

/*! \brief Restore \a this from a \a Dump object.
 *
 * \param[in] d object to read from.
 */
void Part::restore(const Dump *d)
{
    d->restoreInt(m_number);
    d->restoreInt(m_channel);
    d->restoreInt(m_bankSelectMsb);
    d->restoreInt(m_bankSelectLsb);
    d->restoreInt(m_pCh);
    d->restoreInt(m_vol);
    d->restoreInt(m_transpose);
    d->restoreInt(m_oct);
    d->restore((char*)m_preset);
    d->restoreInt(m_keyRangeLower);
    d->restoreInt(m_keyRangeUpper);
    d->restoreInt(m_fadeWidthLower);
    d->restoreInt(m_fadeWidthUpper);
    m_patch.restore(d);
}

/*! \brief Dump \a this to a FILE..
 *
 * \param[in] fp     FILE object to dump to
 * \param[in] prefix Print prefix.
 */
void Part::toTextFile(FILE *fp, const char *prefix) const
{
    fprintf(fp, "%s.preset:%s\n", prefix, m_preset);
    fprintf(fp, "%s.name:%s\n", prefix, m_patch.m_name);
    fprintf(fp, "%s.channel:%d\n", prefix, 1+m_channel);
    fprintf(fp, "%s.vol:%d\n", prefix, m_vol);
    fprintf(fp, "%s.transpose:%d\n", prefix, m_transpose);
    fprintf(fp, "%s.oct:%d\n", prefix, m_oct);
    fprintf(fp, "%s.keyRangeLower:%s\n", prefix, Midi::noteName(m_keyRangeLower));
    fprintf(fp, "%s.keyRangeUpper:%s\n", prefix, Midi::noteName(m_keyRangeUpper));
    fprintf(fp, "%s.fadeWidthLower:%d\n", prefix, m_fadeWidthLower);
    fprintf(fp, "%s.fadeWidthUpper:%d\n", prefix, m_fadeWidthUpper);
}

/*! \brief Save \a this to a \a Dump object.
 *
 * \param[in] d object to dump to.
 */
void Performance::save(const Dump *d)
{
    d->save((const char*)m_name);
    for (int i=0; i<NofParts; i++)
        m_partList[i].save(d);
}

/*! \brief Restore \a this from a \a Dump object.
 *
 * \param[in] d object to read from.
 */
void Performance::restore(const Dump *d)
{
    d->restore((char*)m_name);
    for (int i=0; i<NofParts; i++)
        m_partList[i].restore(d);
}

/*! \brief Set a parameter in Fantom memory.
 *
 * \param[in] addr      Parameter address.
 * \param[in] length    Number of bytes to be written.
 * \param[in] data      Byte string.
 */
void Driver::setParam(const uint32_t addr, const uint32_t length, uint8_t *data)
{
    //wprintw(m_window, "Driver::setParam %08x %08x\n", addr, length);
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
    wprintw(m_window, "sending sysEx %08x %08x\n", addr, length);
    wrefresh(m_window);
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
    //wprintw(m_window, "Driver::getParam %08x %08x\n", addr, length);
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
        wprintw(m_window, "unexpected response, retry\n");
        wrefresh(m_window);
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
    //wprintw(m_window, "received, length = %d\n", i);
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
//    wprintw(m_window, "transpose = %d\n", p->m_transpose);
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
    uint8_t buf[NameLength];
    memset(buf, ' ', sizeof(buf));
    for (int i=0; s[i] && i<NameLength; i++)
    {
        buf[i] = s[i];
    }
    uint32_t addr = 0x11000000 + 0x200000*(uint32_t)part;
    setParam(addr, NameLength, buf);
}

/*! \brief Select a performance in the Fantom.
 *
 * \param[in] performanceIndex  Index of the performance.
 */
void Driver::selectPerformance(uint8_t performanceIndex) const
{
    m_midi->putBytes(Midi::Device::FantomOut,
        Midi::programChange|Fantom::programChangeChannel, performanceIndex);
}

/*! \brief Select performances from memory card in the Fantom.
 *
 * This only works when Fantom is already in performance mode.
 */
void Driver::selectPerformanceFromMemCard() const
{
    m_midi->putBytes(Midi::Device::FantomOut,
        Midi::controller|Fantom::programChangeChannel, 0x00, 85);
    m_midi->putBytes(Midi::Device::FantomOut,
        Midi::controller|Fantom::programChangeChannel, 0x20, 32);
}

void PerformanceList::clear()
{
    if (m_performanceList)
    {
        delete[] m_performanceList;
    }
    m_performanceList = 0;
    m_size = 0;
}

/*! \brief Read contents from a cache file
 *
 * \param[in]   fantomPatchFile     File name.
 * \return      The number of \a Performance entries read, 0 indicates failure.
 */
size_t PerformanceList::readFromCache(const char *fantomPatchFile)
{
    clear();
    Dump d;
    if (d.fopen(fantomPatchFile, O_RDONLY, 0))
    {
        d.restoreInt(m_magic);
        if (m_magic != magic)
            return 0;
        d.restoreInt(m_size);
        m_performanceList = new Fantom::Performance[m_size];
        try
        {
            for (size_t i=0; i<m_size; i++)
            {
                m_performanceList[i].restore(&d);
            }
            d.fclose();
        }
        catch (Error &e)
        {
            clear();
            return 0;
        }
        return m_size;
    }
    return 0;
}

/*! \brief Download contents from Fantom.
 *
 * This is time consuming, so avoid this by trying the cache first.
 *
 * \param[in]   fantom      Fantom driver.
 * \param[in]   win         Curses screen object to log to.
 * \param[in]   nofPerformances   The number of Performances to download.
 */
void PerformanceList::download(Fantom::Driver *fantom, WINDOW *win, size_t nofPerformances)
{
    clear();
    m_size = nofPerformances;
    m_performanceList = new Fantom::Performance[m_size];
    char nameBuf[Fantom::NameLength+1];
    TimeSpec fantomPerformanceSelectDelay((Real)0.01);
    mvwprintw(win, 2, 3, "Downloading Fantom Performance data:");
    for (size_t i=0; i<m_size; i++)
    {
        fantom->selectPerformance(i);
        nanosleep(&fantomPerformanceSelectDelay, NULL);
        fantom->getPerfName(nameBuf);
        g_timer.setTimeout(false);
        mvwprintw(win, 4, 3, "Performance: '%s'", nameBuf);
        Screen::showProgressBar(win, 4, 32, ((Real)i)/m_size);
        wrefresh(win);
        strcpy(m_performanceList[i].m_name, nameBuf);
        for (int j=0; j<Fantom::Performance::NofParts; j++)
        {
            Fantom::Part *hwPart = m_performanceList[i].m_partList+j;
            fantom->getPartParams(hwPart, j);
            bool readPatchParams;
            hwPart->m_number = j;
            hwPart->constructPreset(readPatchParams);
            if (readPatchParams)
            {
                fantom->getPatchName(hwPart->m_patch.m_name, j);
            }
            else
            {
                strcpy(hwPart->m_patch.m_name, "secret GM   ");
            }
            mvwprintw(win, 5, 3, "Part:        '%s'", hwPart->m_patch.m_name);
            Screen::showProgressBar(win, 5, 32, ((Real)(j+1))/Fantom::Performance::NofParts);
            wrefresh(win);
        }
    }
}

/*! \brief Write contents to a cache file, throw on failure.
 *
 * \param[in]   fantomPatchFile     File name.
 */
void PerformanceList::writeToCache(const char *fantomPatchFile)
{
    Dump d;
    if (!d.fopen(fantomPatchFile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR))
    {
        throw(Error("open O_WRONLY|O_CREAT", errno));
    }
    m_magic = magic;
    d.saveInt(m_magic);
    d.saveInt(m_size);
    for (size_t i=0; i<m_size; i++)
    {
        m_performanceList[i].save(&d);
    }
    d.fclose();
}

} // namespace Fantom

