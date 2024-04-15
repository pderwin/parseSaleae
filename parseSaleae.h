#ifndef parseSaleae_h
#define parseSaleae_h

#include <stdlib.h>

#include "log_file.h"
#include "parseSaleae_types.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#include "csv.h"

void csv_process_file       (FILE *fp);
void csv_sample_time_nsecs  (uint32_t nsecs);

void localtime_print        (FILE *fp);
void lookup_init            (char *filename);
void packet_init            (void);
void panel_i2c              (uint32_t byte, time_nsecs_t time_nsecs, uint32_t is_start);
void panel_spi_cmd          (uint32_t byte);
void panel_spi_data         (uint32_t byte);
void print_time_nsecs       (FILE *fp, time_nsecs_t time_nsecs);

uint32_t show_time_stamps   (void);

#endif /* parseJtag_h */
