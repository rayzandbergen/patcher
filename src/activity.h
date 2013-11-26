/*! \file activity.h
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <queue>
#include "timestamp.h"

class ActivityNode
{
public:
    TimeSpec m_expireTime;
    int m_major;
    int m_minor; //!< Not used.
    ActivityNode(int major, int minor, const TimeSpec &now):
        m_major(major), m_minor(minor)
    {
        timeSum(m_expireTime, now, TimeSpec((Real)0.3));
        //ASSERT(std::abs(timeDiffSeconds(now, m_expireTime) - 1.0) < 0.01);
    }
    bool expired(const TimeSpec &now)
    {
        return timeGreaterThanOrEqual(now, m_expireTime);
    }
};

class ActivityList
{
public:
    std::queue<ActivityNode> m_queue;
    int *m_triggerCount;
    int m_majorSize;
    int m_minorSize;
    bool m_dirty;
public:
    ActivityList(int majorSize, int minorSize);
    ~ActivityList();
    bool isDirty() const { return m_dirty; }
    void trigger(int majorIndex, int minorIndex, const TimeSpec &now);
    bool nextExpiry(TimeSpec &ts) const;
    bool update(const TimeSpec &now);
    void get(bool *b);
};

#endif
