#pragma once

#include <stdint.h>
#include "parseSaleae_types.h"

typedef struct frame_s {

   time_nsecs_t
      time_nsecs;

   int32_t
      sample;

} frame_t;
