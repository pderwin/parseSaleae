#pragma once

#include "parseSaleae_types.h"

typedef struct frame_s {

   time_nsecs_t
      time_nsecs;

   unsigned int sample;

} frame_t;
