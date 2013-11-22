/*! \file activity.h
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ACTIVITY_H
#define ACTIVITY_H
#include "timestamp.h"

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
};

#endif
