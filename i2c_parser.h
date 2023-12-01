#pragma once

void i2c_parser_create(parser_t *parser,
		       signal_t *scl,
		       signal_t *sda,
		       void (*parse_packet)(parser_t *parser, time_nsecs_t time_nsecs, uint8_t *packet, uint32_t packet_count)  );
