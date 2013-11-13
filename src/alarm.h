/*! \file alarm.h
 *  \brief Contains an alarm handler, to be used as a watchdog timer.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef ALARM_H
#define ALARM_H
//! \brief Alarm handler state.
class Alarm {
public:
    bool m_doTimeout;
    bool m_globalTimeout;
    Alarm();
};
extern Alarm g_alarm;
extern void setAlarmHandler();
#endif
