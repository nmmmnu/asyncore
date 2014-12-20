#ifndef _SB_H
#define _SB_H

#include <stdint.h>

typedef struct{
	uint32_t buffer_alloc_size;
	uint32_t buffer_size;
	char *buffer;
} SB;

void sb_clear(SB *sb);
void sb_free(SB *sb);

SB *sb_addchar(SB *sb, char a);
SB *sb_addstr(SB *sb, const char *s);
SB *sb_add(SB *sb, const char *buffer, uint32_t buffer_size);

const char *sb_get(SB *sb);

#endif
