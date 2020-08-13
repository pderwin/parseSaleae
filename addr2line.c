#include <stdio.h>
#include <string.h>

extern void initialize_gdb_bfd   (void);
extern void initialize_inferiors (void);
extern void initialize_progspace (void);
extern void
   info_line_command_c    (const char *arg, int from_tty),
   symbol_file_add      (const char *elf_filename);
extern int  gdb_main_c (int argc, char **argv);
extern void gdb_setup_readline_c(int editing);

void addr2line (unsigned int addr)
{
   char
      buf[128];

   sprintf(buf, "*0x%x", addr);


   /*
    * format the symbol name, and limit the length of the printed field to only 30 characters.
    */
   info_line_command_c(buf, 45);
}

static char
   cmdline[128];


void addr2line_init (char *elf_file)
{
   char
      *fake_argv[2],
      *prc_str = "./process_tarmac";
   int
      fake_argc = 2;

   sprintf(cmdline, "%s %s", prc_str, elf_file);

   fake_argv[0] = cmdline;
   fake_argv[1] = cmdline + strlen(prc_str)+1;
   cmdline[16]=0;

   /*
    * Get gdb initialized, and load up the .elf file symbols.
    */
   gdb_main_c(fake_argc, fake_argv);
}
