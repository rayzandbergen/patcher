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

//! \brief Default constructor.
Patch::Patch()
{
    memset(m_name, ' ', sizeof(m_name));
    m_name[sizeof(m_name)-1] = 0;
}

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
                sprintf(m_preset, "USR %03d", m_bankSelectLsb*128+m_programChange+1);
                break;
            case 32:
            case 33:
                sprintf(m_preset, "CRD %03d", (m_bankSelectLsb-32)*128+m_programChange+1);
                break;
            default:
                sprintf(m_preset, "PR%c %03d", (m_bankSelectLsb-64+'A'), m_programChange+1);
                break;
        }
    }
    else if (m_bankSelectMsb == 86)
    {
        switch (m_bankSelectLsb)
        {
            case 0:
                sprintf(m_preset, "USRr%03d", m_bankSelectLsb*128+m_programChange+1);
                break;
            case 32:
                sprintf(m_preset, "CRDr%03d", (m_bankSelectLsb-32)*128+m_programChange+1);
                break;
            default:
                sprintf(m_preset, "PRr%c%03d", (m_bankSelectLsb-64+'A'), m_programChange+1);
                break;
        }
    }
    else if (m_bankSelectMsb == 120)
    {
        sprintf(m_preset, " GMr%03d", m_bankSelectLsb*128+m_programChange+1);
    }
    else if (m_bankSelectMsb == 121)
    {
        sprintf(m_preset, " GM %03d", m_bankSelectLsb*128+m_programChange+1);
    }
    else
    {
        sprintf(m_preset, "?%d-%d-%d?", m_bankSelectMsb, m_bankSelectLsb, m_programChange);
    }
}

/*! \brief Save \a this to a \a Dump object.
 *
 * \param[in] d object to dump to.
 */
void Part::save(const Dump *d)
{
    d->saveInt(m_channel);
    d->saveInt(m_bankSelectMsb);
    d->saveInt(m_bankSelectLsb);
    d->saveInt(m_programChange);
    d->saveInt(m_volume);
    d->saveInt(m_transpose);
    d->saveInt(m_octave);
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
    d->restoreInt(m_channel);
    d->restoreInt(m_bankSelectMsb);
    d->restoreInt(m_bankSelectLsb);
    d->restoreInt(m_programChange);
    d->restoreInt(m_volume);
    d->restoreInt(m_transpose);
    d->restoreInt(m_octave);
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
    fprintf(fp, "%s.volume:%d\n", prefix, m_volume);
    fprintf(fp, "%s.transpose:%d\n", prefix, m_transpose);
    fprintf(fp, "%s.octave:%d\n", prefix, m_octave);
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

//! \brief Default constructor.
Performance::Performance()
{
    memset(m_name, ' ', sizeof(m_name));
    m_name[sizeof(m_name)-1] = 0;
}

} // namespace Fantom

