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

/*! \brief This object knows wether or not is has expired.
 *
 * Expiration time is set to 0.15 seconds after its construction.
 */
class ActivityTrigger
{
    TimeSpec m_expireTime;      //!<    Absolute expiration time.
    int m_major;                //!<    Major index of the trigger.
    int m_minor;                //!<    Minor index of the tigger, not used.
public:
    //! \brief Absolute expiration time.
    const TimeSpec &expireTime() const { return m_expireTime; }
    //! \brief Major index of the trigger.
    int major() const { return m_major; }
    /*! \brief Construct a new trigger.
     *
     * \param[in] major     Major index of the trigger.
     * \param[in] minor     Minor index of the trigger, not used.
     * \param[in] now       The current time.
     */
    ActivityTrigger(int major, int minor, const TimeSpec &now):
        m_major(major), m_minor(minor)
    {
        timeSum(m_expireTime, now, TimeSpec((Real)0.15));
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
    std::queue<ActivityTrigger> m_queue;    //!<    Queue of triggers.
    int *m_triggerCount;                    //!<    List of trigger counters.
    int m_majorSize;                        //!<    Major size of the list.
    int m_minorSize;                        //!<    Minor size of the list, not used.
    int *m_noteOnCount;                     //!<    List of note on counters.
    bool m_dirty;                           //!<    True if any change since last \a get().
public:
    //! \brief Activity state.
    enum State { off, event, on };
    //! \brief Query trigger count for a given major index.
    int triggerCount(int major) const { return m_triggerCount[major]; }
    ActivityList(int majorSize, int minorSize);
    ~ActivityList();
    State get(int majorIndex);
    //! \brief Returns true if any change since last \a get().
    bool isDirty() const { return m_dirty; }
    void trigger(int majorIndex, int minorIndex, bool noteOn, const TimeSpec &now);
    bool nextExpiry(TimeSpec &ts) const;
    bool update(const TimeSpec &now);
    void get(bool *b);
    void clear(int majorIndex);
    void clear();
};

#endif
