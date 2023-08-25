#pragma once

#include "parser.h"

void lbtrace_packet_add_word(parser_t *parser, time_nsecs_t time_nsecs, uint32_t word);
void lbtrace_packet_parse   (parser_t *parser);
void lbtrace_tag_scan       (void);
