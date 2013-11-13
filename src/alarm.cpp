/*! \file alarm.cpp
 *  \brief Contains an alarm handler, to be used as a watchdog timer.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "alarm.h"
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include "error.h"
Alarm g_alarm;  //!< Global watchdog state.

Alarm::Alarm(): m_doTimeout(true), m_globalTimeout(false) { }

//! \brief Alarm handler function, to be attached to a signal.
void alarmHandler(int dummy)
{
    (void)dummy;
    if (g_alarm.m_doTimeout)
        g_alarm.m_globalTimeout = 1;
}

//! \brief Set alarm handler, which will act as a watch dog timer.
void setAlarmHandler()
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
    struct itimerval it;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
#ifdef NO_TIMEOUT
    // well, actually a very long timeout ...
    it.it_value.tv_sec = 3600;
#else
    it.it_value.tv_sec = 2;
#endif
    it.it_value.tv_usec = 500*1000;
    if (setitimer(ITIMER_REAL, &it, 0) < 0)
    {
        throw(Error("setitimer", errno));
    }
}

