#include <stdio.h>
#include <string.h>
#include "addr2line.h"
#include "gdb.h"

extern int  gdb_main_c (int argc, char **argv);

void addr2line (parser_t *parser, unsigned int addr)
{
    char
	buf[128];
    gdb_line_info_t
	gdb_line_info;

    DECLARE_LOG_FP;

    sprintf(buf, "*0x%x", addr);

    /*
     * format the symbol name, and limit the length of the printed field to only 30 characters.
     */
    info_line_command_c(&gdb_line_info, buf);

    fprintf(log_fp, "( %s - Line: %d, %s )",
	    gdb_line_info.function_name,
	    gdb_line_info.lineno,
	    gdb_line_info.source_file_name);
}

/*-------------------------------------------------------------------------
 *
 * name:        addr2line_init
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void addr2line_init (char *elf_file)
{
    char
	*fake_argv[4];
    int
	fake_argc = 4;

    fake_argv[0] =  "./parseSaleae";
    fake_argv[1] = "-q";
    fake_argv[2] = "--nx";
    fake_argv[3] = strdup(elf_file);

    /*
     * Get gdb initialized, and load up the .elf file symbols.
     */
    gdb_main_c(fake_argc, fake_argv);
}
