#include "patcher.h"
#include "timestamp.h"

void getTime(struct timespec *now)
{
    clock_gettime(CLOCK_REALTIME, now);
}

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

Real timeDiffSeconds(const struct timespec *then, const struct timespec *now)
{
    struct timespec diff;
    timeDiff(&diff, then, now);
    return diff.tv_sec + 1e-9 * diff.tv_nsec;
}

