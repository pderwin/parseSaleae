#pragma once

void     tag_atexit (void);
void     tag_find(unsigned int tag);
uint32_t tag_scan(uint32_t max_str_count, char *tag_strs[]);
