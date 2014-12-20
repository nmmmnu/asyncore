#include "sb.h"

#include <stdlib.h>
#include <string.h>

SB *sb_add(SB *sb, const char *src, uint32_t src_size){
	if (src_size == 0)
		return sb;

	uint32_t new_size = sb->buffer_size + src_size;

	char *new_buffer = realloc(sb->buffer, new_size);

	if (new_buffer == NULL)
		return sb;

	memcpy( & new_buffer[ sb->buffer_size ], src, src_size);

	sb->buffer = new_buffer;
	sb->buffer_size = new_size;

	return sb;
}

SB *sb_addchar(SB *sb, char a){
	return sb_add(sb, & a, 1);
}

SB *sb_addstr(SB *sb, const char *s){
	return sb_add(sb, s, strlen(s));
}

const char *sb_get(SB *sb){
	return sb->buffer;
}

void sb_free(SB *sb){
	free(sb->buffer);
	sb_clear(sb);
}

void sb_clear(SB *sb){
	memset(sb, 0, sizeof(*sb));
}
