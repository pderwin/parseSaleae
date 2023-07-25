#pragma once

#include "log_file.h"
#include "parser.h"
#include "parseSaleae_types.h"

void hdr              (parser_t *parser, time_nsecs_t time_nsecs, char *h);
void hdr_with_lineno  (parser_t *parser, time_nsecs_t time_nsecs, char *group_str, char *h, unsigned int lineno);
void print_time_nsecs (FILE *fp, time_nsecs_t time_nsecs);
