/*! \file activity.h
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ACTIVITY_H
#define ACTIVITY_H
#include <queue>
#include "timestamp.h"

#ifdef FAKE_STL // set in PREDEFINED in doxygen config
namespace std { /*! \brief STL queue */ template <class T> class queue {
        public T entry[2]; /*!< Entry. */ }; }
#endif

/*! \brief Keeps track of a trigger until it expires.
 *
 * A trigger expires 0.3 seconds after its creation.
 */
class ActivityTrigger
{
public:
    TimeSpec m_expireTime;      //!<    Absolute expiration time.
    int m_major;                //!<    Major index of the trigger.
    int m_minor;                //!<    Minor index of the tigger, not used.
    /*! \brief Construct a new trigger.
     *
     * \param[in] major     Major index of the trigger.
     * \param[in] minor     Minor index of the trigger, not used.
     * \param[in] now       The current time.
     */
    ActivityTrigger(int major, int minor, const TimeSpec &now):
        m_major(major), m_minor(minor)
    {
        timeSum(m_expireTime, now, TimeSpec((Real)0.3));
    }
    /*! \brief Expiration query
     *
     * \param[in]   now     The current time.
     * \return  True if expired.
     */
    bool expired(const TimeSpec &now)
    {
        return timeGreaterThanOrEqual(now, m_expireTime);
    }
};

//! \brief  Manage a list of ActivityTriggers.
class ActivityList
{
public:
    std::queue<ActivityTrigger> m_queue;    //!<    Queue of triggers.
    int *m_triggerCount;                    //!<    List of trigger counters.
    int m_majorSize;                        //!<    Major size of the list.
    int m_minorSize;                        //!<    Minor size of the list, not used.
    bool m_dirty;                           //!<    True if any change since last \a get().
public:
    ActivityList(int majorSize, int minorSize);
    ~ActivityList();
    //! \brief Returns true if any change since last \a get().
    bool isDirty() const { return m_dirty; }
    void trigger(int majorIndex, int minorIndex, const TimeSpec &now);
    bool nextExpiry(TimeSpec &ts) const;
    bool update(const TimeSpec &now);
    void get(bool *b);
};

#endif
