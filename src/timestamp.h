/*! \file timestamp.h
 *  \brief Contains function prototypes to handle timestamps.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <time.h>
#include "patcher.h"
void getTime(struct timespec *now);
Real timeDiffSeconds(const struct timespec *then, const struct timespec *now);
void timeDiff(struct timespec *diff, const struct timespec *then, const struct timespec *now);
#endif

