#pragma once

#include "parser.h"

void bootloader_i2c(parser_t *parser, uint32_t byte, uint32_t is_start, time_nsecs_t time_nsecs, uint32_t event_count);
