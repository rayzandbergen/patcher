/*! \file timestamp.cpp
 *  \brief Contains functions to handle timestamps.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "patcher.h"
#include "timestamp.h"

/*! \brief Obtains the current time.
 *
 * \param[out] now      The current time from CLOCK_REALTIME.
 */
void getTime(TimeSpec &now)
{
    clock_gettime(CLOCK_REALTIME, &now);
}

/*! \brief Calculates a time difference.
 *
 * \param[out] diff     The time difference.
 * \param[in]  then     Previous time.
 * \param[in]  now      More recent time.
 */
void timeDiff(TimeSpec &diff, const TimeSpec &then, const TimeSpec &now)
{
    diff.tv_sec = now.tv_sec - then.tv_sec;
    if (now.tv_nsec < then.tv_nsec)
    {
        diff.tv_nsec = ((time_t)1000000000 - then.tv_nsec) + now.tv_nsec;
        diff.tv_sec--;
    }
    else
    {
        diff.tv_nsec = now.tv_nsec - then.tv_nsec;
    }
}

/*! \brief Adds to Timespecs.
 *
 * \param[out] sum      The summed time.
 * \param[in]  a        Absolute time or time interval.
 * \param[in]  b        Absolute time or time interval.
 */
void timeSum(TimeSpec &sum, const TimeSpec &a, const TimeSpec &b)
{
    sum.tv_sec = a.tv_sec + b.tv_sec;
    sum.tv_nsec = a.tv_nsec + b.tv_nsec;
    if (sum.tv_nsec >= (time_t)1000000000)
    {
        sum.tv_nsec -= (time_t)1000000000;
        sum.tv_sec++;
    }
}

/*! \brief Test 'greater than or equal' inequality of TimeSpecs.
 *
 * \param[in]  a        Left hand side.
 * \param[in]  b        Right hand side.
 * \return     True if a >= b.
 */
bool timeGreaterThanOrEqual(const TimeSpec &a, const TimeSpec &b)
{
    if (a.tv_sec == b.tv_sec)
        return a.tv_nsec >= b.tv_nsec;
    return a.tv_sec > b.tv_sec;
}

/*! \brief Calculates a time difference in seconds.
 *
 * \param[in]  then     Previous time.
 * \param[in]  now      More recent time.
 * \return      The time difference in seconds.
 */
Real timeDiffSeconds(const TimeSpec &then, const TimeSpec &now)
{
    TimeSpec diff;
    timeDiff(diff, then, now);
    return diff.tv_sec + ((Real)1e-9) * diff.tv_nsec;
}

