/*! \file timer.cpp
 *  \brief Contains an timer handler, to be used as a watchdog timer.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "timer.h"
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <cmath>
#include "error.h"

Timer g_timer;

//! \brief Alarm handler function, to be attached to a signal.
void alarmHandler(int dummy)
{
    (void)dummy;
    if (g_timer.m_doTimeout)
        g_timer.m_globalTimeout = 1;
}

//! \brief Set alarm handler, which will act as a watch dog timer.
void Timer::setTimeout(Real seconds)
{
    struct sigaction action;
    action.sa_sigaction = 0;
    action.sa_handler = alarmHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_restorer = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        throw(Error("sigaction", errno));
    }
    Real floorSec = floor(seconds);
    struct itimerval it;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
#ifdef NO_TIMEOUT
    // well, actually a very long timeout ...
    it.it_value.tv_sec = 3600;
#else
    it.it_value.tv_sec = (time_t)floorSec;
#endif
    it.it_value.tv_usec = (time_t)(seconds-floorSec);
    if (setitimer(ITIMER_REAL, &it, 0) < 0)
    {
        throw(Error("setitimer", errno));
    }
}

//! \brief Read with timeout.
void Timer::read(int fd, void *buf, size_t count)
{
    do
    {
        ssize_t n = ::read(fd, buf, count);
        if (timedout())
        {
            throw(TimeoutError("read timeout"));
        }
        if (n < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                throw(Error("read", errno));
            }
            n = 0;
        }
        buf = (void*)(((char*)buf) + n);
        count -= n;
    } while (count);
}

//! \brief Write with timeout.
void Timer::write(int fd, const void *buf, size_t count)
{
    for (;;)
    {
        ssize_t rv = ::write(fd, buf, count);
        if (timedout())
        {
            throw(TimeoutError("write timeout"));
        }
        if (rv == -1)
        {
            if (errno != EINTR)
            {
                throw(Error("write", errno));
            }
            rv = 0;
        }
        if (rv == (ssize_t)count)
        {
            break;
        }
        else
        {
            count -= rv;
            buf = (char*)buf + count;
        }
    }
}

