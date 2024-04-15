#pragma once

#include <stdint.h>
#include "parser.h"

void addr2line              (parser_t *parser, uint32_t address);
void addr2line_init         (char *elf_file);
