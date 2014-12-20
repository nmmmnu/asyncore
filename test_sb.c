#include "sb.h"

#include <string.h>
#include <stdio.h>

int main(){
	SB sb_place;
	SB *sb = &sb_place;

	sb_clear(sb);

	sb_addstr(sb, "All");
	sb_addchar(sb, ' ');
	sb_addstr(sb, "test");
	sb_addchar(sb, ' ');
	sb_addstr(sb, "passed!");
	sb_addchar(sb, ' ');
	sb_addstr(sb, "You are awesome :)");

	printf("%s\n", sb->buffer);

	return 0;
}

