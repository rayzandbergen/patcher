#ifndef ALARM_H
#define ALARM_H
class Alarm {
public:
    bool m_doTimeout;
    bool m_globalTimeout;
    Alarm();
};
extern Alarm g_alarm;
extern int setAlarmHandler();
#endif
