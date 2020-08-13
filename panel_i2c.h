#pragma once

#include "parseSaleae.h"

void panel_i2c_connect (void);
void panel_i2c_irq     (frame_t *frame, int state);
