/*! \file activity.cpp
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include "timestamp.h"
#include "activity.h"
#include "error.h"
#include "patchercore.h"

/*! \brief Constructor.
 *
 * Contains a binary tree of activity flags to keep track of at most
 * majorSize * minorSize times.
 *
 * \param[in]   majorSize   Major size, i.e. query granularity.
 * \param[in]   minorSize   Minor size.
 */
ActivityList::ActivityList(int majorSize, int minorSize):
    m_majorSize(majorSize), m_minorSize(minorSize), m_dirty(true)
{
    m_triggerCount = new int[m_majorSize];
    m_noteOnCount = new int[m_majorSize];
    for (int i=0; i<m_majorSize; i++)
    {
        m_triggerCount[i] = 0;
        m_noteOnCount[i] = 0;
    }
}
/*! \brief Destructor.
 */
ActivityList::~ActivityList()
{
    delete[] m_triggerCount;
    delete[] m_noteOnCount;
}

/*! \brief Stores the current time at given major/minor location and updates the activity tree.
 *
 * \param[in]   majorIndex   Major location index.
 * \param[in]   minorIndex   Minor location index.
 * \param[in]   noteOn       Set to true if this is a note on trigger.
 * \param[in]   now          Current time.
 */
void ActivityList::trigger(int majorIndex, int minorIndex, bool noteOn, const TimeSpec &now)
{
    m_queue.push(ActivityTrigger(majorIndex, minorIndex, now));
    m_triggerCount[majorIndex] += 1;
    if (minorIndex != 0)
    {
        if (noteOn)
        {
            m_noteOnCount[majorIndex]++;
            m_dirty = m_dirty || m_noteOnCount[majorIndex] == 1;
        }
        else
        {
            if (m_noteOnCount[majorIndex] > 0)
            {
                m_noteOnCount[majorIndex]--;
                m_dirty = m_noteOnCount[majorIndex] == 0;
            }
        }
    }
    m_dirty = m_dirty || m_triggerCount[majorIndex] == 1;
}

/*! \brief  Returns the next expiry time, if any.
 *
 * \param[out]  expireTime  Next expiry time
 * \return True if there is a next expiry time, false otherwise.
 */
bool ActivityList::nextExpiry(TimeSpec &expireTime) const
{
    if (m_queue.empty())
        return false;
    expireTime = m_queue.front().expireTime();
    return true;
}

/*! \brief Update activity list with current time.
 *
 * This function clears activity slots that are expired.
 *
 * \param[in]   now      Current time.
 * \return  \sa getDirty().
 */
bool ActivityList::update(const TimeSpec &now)
{
    while(!m_queue.empty())
    {
        if (m_queue.front().expired(now))
        {
            int major = m_queue.front().major();
            m_triggerCount[major] -= 1;
            m_dirty = m_dirty || (m_triggerCount[major] == 0);
            m_queue.pop();
        }
        else
            break;
    }
    return m_dirty;
}

/*! \brief Get list of activity flags, and clear the dirty flag.
 *
 * \param[out]  b    Bool array, must be at least majorSize entries.
 */
void ActivityList::get(bool *b)
{
    for (int i=0; i<m_majorSize; i++)
        b[i] = !!m_triggerCount[i];
    m_dirty = false;
}

/*! \brief Get activity state, and clear the dirty flag.
 *
 * \param[in]  majorIndex   Major index.
 */
ActivityList::State ActivityList::get(int majorIndex)
{
    m_dirty = false;
    if (m_triggerCount[majorIndex] != 0)
        return event;
    if (m_noteOnCount[majorIndex] > 0)
        return on;
    return off;
}

/*! \brief Clears all activity for majorIndex.
 *
 * \param[in]  majorIndex    Major index.
 */
void ActivityList::clear(int majorIndex)
{
    m_triggerCount[majorIndex] = 0;
    m_noteOnCount[majorIndex] = 0;
    // clean queue by copying nonmatching elements, then swapping
    std::queue<ActivityTrigger> cleanedQueue;
    while (!m_queue.empty())
    {
        if (m_queue.front().major() != majorIndex)
            cleanedQueue.push(m_queue.front());
        m_queue.pop();
    }
    swap(m_queue, cleanedQueue);
    m_dirty = true;
}

/*! \brief Clears all activity.
 *
 */
void ActivityList::clear()
{
    while (!m_queue.empty())
        m_queue.pop();
    for (int i=0; i<m_majorSize; i++)
        m_triggerCount[i] = 0;
    m_dirty = true;
}
