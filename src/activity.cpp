/*! \file activity.cpp
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdio.h>
//#include <iostream>
#include <stdlib.h>
#include <vector>
#include "timestamp.h"
#include "activity.h"
#include "error.h"
#include "patcher.h"

namespace {

//! \brief calculates ceil(log2(x))
int ceilLog2(int x)
{
    int y = 0;
    for (int testVal = 1; x > testVal; y++, testVal<<=1);
    return y;
}

} // anonymous namespace

/* \brief Binary activity tree node.
 *
 * Active means 'triggered recently'. Since the vast majority of
 * slots is inactive at any given time, a binary tree is used to
 * get to the few active slots quickly.
 */
class ActivityNode
{
public:
    TimeSpec m_ts;      //!<    Time of last trigger, valid for leaf nodes only, so slightly wasteful.
    bool m_active;      //!<    This node, or at least 1 child is active.
    //! \brief Default constructor.
    ActivityNode(): m_active(false) { }
};

/*! \brief Constructor.
 *
 * Contains a binary tree of activity flags to keep track of at most
 * majorSize * minorSize times.
 *
 * \param[in]   majorSize   Major size, i.e. query granularity.
 * \param[in]   minorSize   Minor size.
 * \param[in]   ts          Some valid time.
 */
ActivityList::ActivityList(int majorSize, int minorSize): m_majorSize(majorSize), m_dirty(true)
{
    m_majorBits = ceilLog2(majorSize);
    m_minorBits = ceilLog2(minorSize);
    m_size = 1 << (m_majorBits + m_minorBits);
    // we waste the first entry to simplify
    // the index expressions.
    // Leaf nodes start at offset m_size.
    m_nodeList = new ActivityNode[2*m_size];
}

/*! \brief Destructor.
 */
ActivityList::~ActivityList()
{
    delete[] m_nodeList;
}

/*! \brief Stores the current time at given major/minor location and updates the activity tree.
 *
 * \param[in]   major   Major location index.
 * \param[in]   minor   Minor location index.
 * \param[in]   ts      Current time.
 */
void ActivityList::trigger(int major, int minor, const TimeSpec &ts)
{
    size_t i = m_size + offset(major, minor);
    m_nodeList[i].m_ts = ts;
    if (!m_nodeList[i].m_active)
        m_dirty = true;
    do
    {
        //std::cout << "trigger " << i << "\n";
        m_nodeList[i].m_active = true;
        i >>= 1;
    } while (i >= m_root);
}

/*! \brief Clear all acitvity.
 *
 * \param[in]   idx     Tree index at which to start.
 */
void ActivityList::clear(size_t idx)
{
    if (m_nodeList[idx].m_active)
    {
        m_nodeList[idx].m_active = false;
        if (idx < m_size)
        {
            size_t child = idx << 1;
            clear(child);
            clear(child+1);
        }
    }
}

/*! \brief Clear all acitvity.
 */
void ActivityList::clear()
{
    clear(m_root);
    m_dirty = true; // really, since this is a forced status change.
}

/*! \brief Update activity list with current time.
 *
 * This function clears activity slots that are expired.
 *
 * \param[in]   ts      Current time.
 * \param[in]   idx     Tree index at which to start, default is root.
 */
void ActivityList::update(const TimeSpec &ts, size_t idx)
{
    if (m_nodeList[idx].m_active)
    {
        if (idx >= m_size)
        {
            //std::cout << "checking " << idx << "\n";
            bool recentTrigger = 
                timeDiffSeconds(m_nodeList[idx].m_ts, ts) < (Real)0.3;
            if (!recentTrigger)
            {
                m_dirty = true;
                m_nodeList[idx].m_active = false;
            }
        }
        else
        {
            size_t child = idx << 1;
            update(ts, child);
            update(ts, child+1);
            m_nodeList[idx].m_active = m_nodeList[child].m_active
                    || m_nodeList[child+1].m_active;
        }
    }
}

/*! \brief Get list of activity flags, and clear the dirty flag.
 *
 * \param[out]  activity    Bool array, must be at least majorSize entries.
 */
void ActivityList::get(bool *activity)
{
    size_t majorOffset = 1 << m_majorBits;
    for (size_t i=0; i<m_majorSize; i++)
    {
        activity[i] = m_nodeList[majorOffset+i].m_active;
    }
    m_dirty = false;
}

