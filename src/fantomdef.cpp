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

Part::Part(): m_channel(255)
{
    strcpy(m_preset, "?");
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

//! \brief Default constructor.
Performance::Performance()
{
    memset(m_name, ' ', sizeof(m_name));
    m_name[sizeof(m_name)-1] = 0;
}

} // namespace Fantom

