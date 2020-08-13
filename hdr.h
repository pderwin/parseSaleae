#pragma once

#include "log_file.h"
#include "parseSaleae_types.h"

void hdr                   (log_file_t *lf, time_nsecs_t time_nsecs, char *h);
void hdr_with_lineno       (log_file_t *lf, time_nsecs_t time_nsecs, char *h, unsigned int lineno);
