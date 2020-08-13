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


typedef struct frame_s {

   time_nsecs_t
      time_nsecs;

   unsigned int sample;

} frame_t;

typedef struct parser_s parser_t;

typedef struct signal_s {
    char     *name;
    uint32_t *val_p;
    uint32_t format; // format to use when calling csv_find.
    uint32_t find_rc;
} signal_t;

struct parser_s {
    char *name;

    uint32_t (*connect)      (void);
    void (*process_frame)(frame_t *frame);

    uint32_t enable;

    signal_t *signals;

    char       *log_file;
    log_file_t *lf;

    uint32_t sample_time_nsecs;

    parser_t *next;
};

typedef enum
    {
     USE_115K_BAUD,
     USE_460K_BAUD
    } use_baud_e;

void addr2line              (unsigned int address);
void addr2line_init         (char *elf_file);
void bootloader_i2c         (uint32_t byte, uint32_t is_start, time_nsecs_t time_nsecs, uint32_t event_count);
void clock_check            (frame_t *jf);
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
void parser_connect         (void);
void parser_process_frame   (frame_t *frame);
void parser_register        (parser_t *parser);
void print_time_nsecs       (FILE *fp, time_nsecs_t time_nsecs);
void rts_process            (frame_t *frame);

void st25dv_connect         (void);
void st25dv_i2c             (uint32_t byte, time_nsecs_t time_nsecs);
void st25dv_start           (void);
void st25dv_stop            (void);

void     tag_find              (unsigned int tag);
void     tag_dump              (void);
void     tag_init              (void);
uint32_t tag_scan              (uint32_t max_str_count, char *tag_strs[]);

unsigned int verbose;

#endif /* parseJtag_h */
