/*! \file activity.h
 *  \brief Contains an object to keep activity flags.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ACTIVITY_H
#define ACTIVITY_H
#include "timestamp.h"
//! \brief This class administers a list of activity flags.
class Activity
{
    static const int nofSlots = 16;     //!<    Number of slots.
    bool m_active[nofSlots];            //!<    Activity flags.
    struct timespec m_t0[nofSlots];     //!<    Time of last activity.
    bool m_dirty;                       //!<    Dirty flag, set if any change in activity since last \a clean().
public:
    void clean() { m_dirty = false; }   //!<    Clear dirty flag.
    bool isDirty() const { return m_dirty; } //!<    Query dirty flag.
    Activity();                         //!<    Default constructor
    void set(int idx);                  //!<    Set activity flag for slot \a idx.
    bool get(int idx);                  //!<    Get activity flag for slot \a idx.
};
#endif
