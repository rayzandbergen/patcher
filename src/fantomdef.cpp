/*! \file fantomdef.cpp
 *  \brief Contains classes to store patches/parts/performances (Roland-speak) from the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "fantomdef.h"
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
