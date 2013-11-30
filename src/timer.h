/*! \file timer.h
 *  \brief Contains an timer handler, to be used as a watchdog timer.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ALARM_H
#define ALARM_H
#include "error.h"
#include "patchercore.h"

//! \brief Timeout exception.
class TimeoutError: public Error
{
public:
    //! \brief Construct from a simple message string.
    TimeoutError(const char *msg): Error(msg) { }
};

void alarmHandler(int dummy);

/*! \brief Timer class.
 *
 * This class can deliver timeout signals, so system calls can be
 * interrupted after a set timeout.
 * It also contains read and write functions that throw on timeout.
 */
class Timer
{
friend void alarmHandler(int);
    int m_watchdogCount;    //!<    Watchdog counter.
public:
    void setTimeout(Real seconds, int watchdogCounter);
    //! \brief Reset the watchdog counter.
    void resetWatchdog(int count) { m_watchdogCount = count; }
    //! \brief Timeout check after interrupted system call.
    bool timedout() { return m_watchdogCount == 0; }
    void write(int fd, const void *buf, size_t count);
    void read(int fd, void *buf, size_t count);
    Timer(): m_watchdogCount(1) { }
};

extern Timer g_timer; //!< Global timer object.

#endif
