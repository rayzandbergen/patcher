/*! \file timestamp.h
 *  \brief Contains function prototypes to handle timestamps.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <time.h>
#include <cmath>
#include "patcher.h"
//! \brief A wrapper class for struct timespec from <time.h>.
class TimeSpec: public timespec
{
public:
    /*! \brief Constructor.
     *
     * \param[in] sec   Seconds.
     * \param[in] nsec  Nanoseconds.
     */
    TimeSpec(time_t sec = 0, long int nsec = 0)
    {
        tv_sec=sec;
        tv_nsec=nsec;
    }
    /*! \brief Constructor.
     *
     * \param[in] sec   Seconds.
     */
    TimeSpec(Real sec)
    {
        Real floorSec = floor(sec);
        tv_sec = (time_t)floorSec;
        tv_nsec = (time_t)((sec - floorSec)*(Real)1e+9);
    }
};
void getTime(TimeSpec &now);
Real timeDiffSeconds(const TimeSpec &then, const TimeSpec &now);
void timeDiff(TimeSpec &diff, const TimeSpec &then, const TimeSpec &now);
#endif

