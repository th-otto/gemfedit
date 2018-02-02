#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
	int i;
	unsigned long ch;
	
	for (i = 1; i < argc; i++)
	{
		ch = strtol(argv[i], NULL, 0);
		if (ch >= 0x10000)
		{
			fprintf(stderr, "%lx: out of range\n", ch);
		} else if (ch >= 0x800)
		{
			printf("0x%lx: \\%03lo\\%03lo\\%03lo \\x%0lx\\x%0lx\\x%02lx\n", ch,
				((ch >> 12) & 0x0f) | 0xe0,
				((ch >> 6) & 0x3f) | 0x80,
				(ch & 0x3f) | 0x80,
				((ch >> 12) & 0x0f) | 0xe0,
				((ch >> 6) & 0x3f) | 0x80,
				(ch & 0x3f) | 0x80);
		} else if (ch >= 0x80)
		{
			printf("0x%lx: \\%03lo\\%03lo \\x%0lx\\x%0lx\n", ch,
				((ch >> 6) & 0x3f) | 0xc0,
				(ch & 0x3f) | 0x80,
				((ch >> 6) & 0x3f) | 0xc0,
				(ch & 0x3f) | 0x80);
		} else if (ch >= 0x20 && ch != 0x7f)
		{
			printf("0x%lx: %c\n", ch, (int)ch);
		} else
		{
			printf("0x%lx: \\%03lo \\x%0lx\n", ch, ch, ch);
		}	
	}
	
	return 0;
}
