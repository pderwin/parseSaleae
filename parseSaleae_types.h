#pragma once

#define CONSTRUCTOR __attribute__ ((constructor))

/*
 * time stamp needs to be signed.
 */
typedef long long int time_nsecs_t;
