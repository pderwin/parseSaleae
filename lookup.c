#include <string.h>
#include "parseSaleae.h"

char *elf_filename;

void
   lookup (uint32_t address)
{
   char cmd[512];
   char *cp;
   char lookup[1024];
   unsigned int len;
   FILE *fp;

   if (elf_filename == NULL) {
       static uint32_t first = 1;

       if (first) {

	   first = 0;

	   fprintf(stderr, "%s: ERROR: elf filename not specified\n", __func__);
	   printf("%s: ERROR: elf filename not specified\n", __func__);
       }
       return;
   }

   //   printf("%s: 0x%x -- ", str, address);
   //    addr2line(address);

   //   return;

   sprintf(cmd, "lookup-riscv --elf=%s 0x%x", elf_filename, address);

   //	    printf ("%s", cmd);

   fp = popen(cmd, "r");
   fgets(lookup, sizeof(lookup), fp);
   pclose(fp);

   cp = strtok(lookup, " ");
   cp = strtok(NULL, " ");
   printf("Line %s, ", cp);

   cp = strtok(NULL, "\"");
   cp = strtok(NULL, "\"");

   if (cp) {
      len = strlen(cp);

      if (len > 50) {
	 cp += (len-50);
      }

      printf ("%s", cp);
   }
}

void lookup_init (char *filename)
{
    elf_filename = strdup(filename);
}
