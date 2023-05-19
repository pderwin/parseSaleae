#ifndef parseSaleae_h
#define parseSaleae_h

#include <stdlib.h>

#include "log_file.h"
#include "parseSaleae_types.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#include "csv.h"

/*
 * Used in pluggable parsers
 */
#define falling_edge(__elem) (sample.__elem == 0) && (last_sample.__elem == 1)
#define rising_edge(__elem)  (sample.__elem == 1) && (last_sample.__elem == 0)


typedef struct parser_s parser_t;



typedef enum
    {
     USE_115K_BAUD,
     USE_460K_BAUD
    } use_baud_e;

void addr2line              (unsigned int address);
void addr2line_init         (char *elf_file);
void bootloader_i2c         (uint32_t byte, uint32_t is_start, time_nsecs_t time_nsecs, uint32_t event_count);
void csv_process_file       (FILE *fp);
void csv_sample_time_nsecs  (uint32_t nsecs);
void hex_process_file       (FILE *fp);

void lbtrace_packet_add_word(time_nsecs_t time_nsecs, uint32_t word);
void lbtrace_packet_parse   (void);
void lbtrace_tag_scan       (void);

void localtime_print        (FILE *fp);
void lookup                 (uint32_t address);
void lookup_init            (char *filename);
void packet_init            (void);
void packet_process         (unsigned int idx, time_nsecs_t time_nsecs,
			     unsigned int tag, unsigned int lineno, unsigned int argc, unsigned int argv[]);
void panel_i2c              (uint32_t byte, time_nsecs_t time_nsecs, uint32_t is_start);
void panel_spi_cmd          (uint32_t byte);
void panel_spi_data         (uint32_t byte);
void print_time_nsecs       (FILE *fp, time_nsecs_t time_nsecs);

unsigned int verbose;

#endif /* parseJtag_h */
