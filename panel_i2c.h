#pragma once

#include "parseSaleae.h"

typedef struct frame_s frame_t;

void panel_i2c_connect (void);
void panel_i2c_irq     (frame_t *frame, int state);
