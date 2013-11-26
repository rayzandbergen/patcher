/*! \file activity.h
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <queue>
#include "timestamp.h"
#include "error.h"

class ActivityNode;

/*! \brief Manages activity status flags.
 */
class ActivityList
{
    int m_majorBits;        //!<    Number of bits needed to store major size.
    int m_minorBits;        //!<    Number of bits needed to store minor size.
    size_t m_majorSize;     //!<    Major size.
    size_t m_size;          //!<    2^(m_majorBits + m_minorBits).
    ActivityNode *m_nodeList;   //!<    Flattened binary tree of activity nodes, root is at offset 1.
    bool m_dirty;           //!<    True after any activity change.
    //! \brief Combines major and minor index into a single offset.
    size_t offset(int major, int minor) const
    {
        return (major << m_minorBits) | minor;
    }
    void clear(size_t index);
public:
    static const size_t m_root = 1;    //!<    Root node offset, slot 0 is not used.
    ActivityList(int majorSize, int minorSize);
    ~ActivityList();
    //! \brief Returns true if any change since last \a get().
    bool isDirty() const { return m_dirty; }
    void trigger(int majorIndex, int minorIndex, const TimeSpec &ts);
    void update(const TimeSpec &ts, size_t index = m_root);
    void clear();
    void get(bool *b);
    bool any() const;
};

class ActivityNode2
{
public:
    TimeSpec m_expireTime;
    int m_major;
    int m_minor; //!< Not used.
    ActivityNode2(int major, int minor, const TimeSpec &now):
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

class ActivityList2
{
public:
    std::queue<ActivityNode2> m_queue;
    int *m_triggerCount;
    int m_majorSize;
    int m_minorSize;
    bool m_dirty;
public:
    ActivityList2(int majorSize, int minorSize):
        m_majorSize(majorSize), m_minorSize(minorSize), m_dirty(true)
    {
        m_triggerCount = new int[m_majorSize];
        for (int i=0; i<m_majorSize; i++)
        {
            m_triggerCount[i] = 0;
        }
    }
    ~ActivityList2()
    {
        delete[] m_triggerCount;
    }
    bool isDirty() const { return true||m_dirty; }
    void trigger(int majorIndex, int minorIndex, const TimeSpec &now)
    {
        m_queue.push(ActivityNode2(majorIndex, minorIndex, now));
        m_triggerCount[majorIndex] += 1;
        m_dirty = true;
    }
    bool nextExpiry(TimeSpec &ts) const
    {
        if (m_queue.empty())
            return false;
        ts = m_queue.front().m_expireTime;
        return true;
    }
    bool update(const TimeSpec &now)
    {
        while(!m_queue.empty())
        {
            if (m_queue.front().expired(now))
            {
                int major = m_queue.front().m_major;
                m_triggerCount[major] -= 1;
                m_dirty = m_dirty || true;
                m_queue.pop();
            }
            else
                break;
        }
        return m_dirty;
    }
    void get(bool *b)
    {
        for (int i=0; i<m_majorSize; i++)
            b[i] = !!m_triggerCount[i];
        m_dirty = false;
    }
};

#endif
