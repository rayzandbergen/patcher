/*! \file activity.cpp
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdio.h>
#include "timestamp.h"
#include "activity.h"
#include "error.h"

typedef struct timespec TimeSpec;

Activity::Activity(int nofSlots): m_nofSlots(nofSlots)
{
    m_active = new bool[m_nofSlots];
    m_t0 = new TimeSpec[m_nofSlots];
    clean();
    for (int i=0; i<m_nofSlots; i++)
    {
        m_active[i] = false;
    }
}

Activity::~Activity()
{
    delete[] m_active;
    delete[] m_t0;
}

void Activity::set(int idx)
{
    ASSERT(idx < m_nofSlots);
    if (!m_active[idx])
    {
        m_dirty = true;
        m_active[idx] = true;
    }
    getTime(m_t0 + idx);
}

bool Activity::get(int idx)
{
    ASSERT(idx < m_nofSlots);
    if (!m_active[idx])
        return false;

    struct timespec now;
    getTime(&now);
    if (timeDiffSeconds(m_t0 + idx, &now) > (Real)0.1)
    {
        m_dirty = true;
        m_active[idx] = false;
    }
    return m_active[idx];
}

