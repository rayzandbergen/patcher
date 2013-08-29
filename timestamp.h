#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <time.h>
#include "patcher.h"
void getTime(struct timespec *now);
Real timeDiff(const struct timespec *then, const struct timespec *now);
#endif

