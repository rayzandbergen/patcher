#include <stdio.h>
#include "timestamp.h"
#include "activity.h"
#include "assert.h"

Activity::Activity()
{
    clean();
    for (int i=0; i<nofSlots; i++)
    {
        m_active[i] = false;
    }
}

void Activity::set(int idx)
{
    assert(idx < nofSlots);
    if (!m_active[idx])
    {
        m_dirty = true;
        m_active[idx] = true;
    }
    getTime(m_t0 + idx);
}

bool Activity::get(int idx)
{
    assert(idx < nofSlots);
    if (!m_active[idx])
        return false;

    struct timespec now;
    getTime(&now);
    if (timeDiff(m_t0 + idx, &now) > (Real)0.1)
    {
        m_dirty = true;
        m_active[idx] = false;
    }
    return m_active[idx];
}

