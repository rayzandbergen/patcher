#ifndef ACTIVITY_H
#define ACTIVITY_H
#include "timestamp.h"
class Activity
{
    static const int nofSlots = 16;
    bool m_active[nofSlots];
    struct timespec m_t0[nofSlots];
    bool m_dirty;
public:
    void clean() { m_dirty = false; }
    bool isDirty() const { return m_dirty; }
    Activity();
    void set(int idx);
    bool get(int idx);
};
#endif
