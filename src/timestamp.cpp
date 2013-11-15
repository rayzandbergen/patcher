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
void getTime(struct timespec *now)
{
    clock_gettime(CLOCK_REALTIME, now);
}

/*! \brief Calculates a time difference.
 *
 * \param[out] diff     The time difference.
 * \param[in]  then     Previous time.
 * \param[in]  now      More recent time.
 */
void timeDiff(struct timespec *diff, const struct timespec *then, const struct timespec *now)
{
    diff->tv_sec = now->tv_sec - then->tv_sec;
    if (now->tv_nsec < then->tv_nsec)
    {
        diff->tv_nsec = ((time_t)1000000000 - then->tv_nsec) + now->tv_nsec;
        diff->tv_sec--;
    }
    else
    {
        diff->tv_nsec = now->tv_nsec - then->tv_nsec;
    }
}

/*! \brief Calculates a time difference in seconds.
 *
 * \param[in]  then     Previous time.
 * \param[in]  now      More recent time.
 * \return      The time difference in seconds.
 */
Real timeDiffSeconds(const struct timespec *then, const struct timespec *now)
{
    struct timespec diff;
    timeDiff(&diff, then, now);
    return diff.tv_sec + 1e-9 * diff.tv_nsec;
}

