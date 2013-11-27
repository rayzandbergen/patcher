/*! \file fantomscroller.cpp
 *  \brief Contains an object produces scrolling text on the Fantom screen.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "fantomscroller.h"
#include "networkif.h"
#include "patcher.h"

//! \brief Default constructor.
FantomScreenScroller::FantomScreenScroller(Fantom::Driver *fantomDriver):
    m_enabled(false), m_fantom(fantomDriver), m_offset(0) { }

//! \brief On/off toggle.
void FantomScreenScroller::toggle()
{
    m_enabled = ! m_enabled;
    if (m_enabled)
    {
        m_text = getNetworkInterfaceAddresses();
    }
}

//! \brief Update the screen.
void FantomScreenScroller::update(TimeSpec &now)
{
    if (!enabled())
        return;

    if (timeDiffSeconds(m_lastUpdateTime, now) > 0.333)
    {
        m_lastUpdateTime = now;
        // abuse the fantom screen to print some info
        char buf[Fantom::NameLength];
        for (size_t i=0, j=m_offset; i<sizeof(buf); i++)
        {
            j++;
            if (j >= m_text.length())
                j = 0;
            buf[i] = m_text[j];
        }
        m_fantom->setPartName(0, buf);
        m_offset++;
        if (m_offset >= m_text.size())
            m_offset = 0;
    }
}

