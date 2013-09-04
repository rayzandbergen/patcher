#include "alarm.h"
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
Alarm g_alarm;

Alarm::Alarm(): m_doTimeout(true), m_globalTimeout(false) { }

void alarmHandler(int dummy)
{
    (void)dummy;
    if (g_alarm.m_doTimeout)
        g_alarm.m_globalTimeout = 1;
}

int setAlarmHandler()
{
    struct sigaction action;
    action.sa_sigaction = 0;
    action.sa_handler = alarmHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_restorer = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("sigaction");
        return -3;
    }
    struct itimerval it;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 1;
    it.it_value.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &it, 0) < 0)
    {
        perror("setitimer");
        return -3;
    }
    return 0;
}


