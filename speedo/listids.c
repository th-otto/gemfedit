/*
 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices and 
 * Digital not be used in advertising or publicity pertaining to 
 * distribution of the software without specific, written prior permission.  
 * Network Computing Devices and Digital make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * NETWORK COMPUTING DEVICES AND DIGITAL DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES OR DIGITAL BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Dave Lemke
 */

/*

Copyright 1987, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/*
 * Speedo outline to BFD format converter
 */

#include "linux/libcwrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "speedo.h"

static FILE *fp;

static ufix16 char_index, char_id;

static buff_t font;

static ufix8 *f_buffer;
static ufix8 *c_buffer;

static ufix16 mincharsize;

static char *progname;

static char *fontfile = NULL;

static specs_t specs;

static void usage(void)
{
	fprintf(stderr,
			"Usage: %s fontfile\n",
			progname);
	exit(0);
}

static fix15 read_2b(ufix8 *ptr)
{
	fix15 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

static fix31 read_4b(ufix8 *ptr)
{
	fix31 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

static void process_args(int ac, char **av)
{
	int i;

	for (i = 1; i < ac; i++)
	{
		if (*av[i] == '-')
		{
			usage();
		} else
		{
			fontfile = av[i];
		}
	}
	if (!fontfile)
		usage();
}


int main(int argc, char **argv)
{
	ufix32 i;
	ufix8 tmp[16];
	ufix32 minbufsize;
	const ufix8 *key;
	int first_char_index, num_chars;

	progname = argv[0];
	process_args(argc, argv);
	fp = fopen(fontfile, "r");
	if (!fp)
	{
		fprintf(stderr, "No such font file, \"%s\"\n", fontfile);
		return 1;
	}
	if (fread(tmp, sizeof(ufix8), 16, fp) != 16)
	{
		fprintf(stderr, "error reading \"%s\"\n", fontfile);
		return 1;
	}
	minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
	f_buffer = (ufix8 *) malloc(minbufsize);
	if (!f_buffer)
	{
		fprintf(stderr, "can't get %x bytes of memory\n", minbufsize);
		return 1;
	}
	fseek(fp, 0, SEEK_SET);

	if (fread(f_buffer, sizeof(ufix8), (ufix16) minbufsize, fp) != minbufsize)
	{
		fprintf(stderr, "error reading file \"%s\"\n", fontfile);
		return 1;
	}
	mincharsize = read_2b(f_buffer + FH_CBFSZ);

	c_buffer = (ufix8 *) malloc(mincharsize);
	if (!c_buffer)
	{
		fprintf(stderr, "can't get %x bytes for char buffer\n", mincharsize);
		return 1;
	}
	/* init */
	sp_reset();

	font.org = f_buffer;
	font.no_bytes = minbufsize;

	key = sp_get_key(&font);
	if (key == NULL)
	{
#if 0
		fprintf(stderr, "Non-standard encryption for \"%s\"\n", fontfile);
		return 1;
#endif
	} else
	{
		sp_set_key(key);
	}
	
	first_char_index = read_2b(f_buffer + FH_FCHRF);
	num_chars = read_2b(f_buffer + FH_NCHRL);

	/* set up specs */
	/* Note that point size is in decipoints */
	specs.pfont = &font;
	/* XXX beware of overflow */
	specs.xxmult = 0;
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = 0;
	specs.yoffset = 0L << 16;
	specs.flags = 0;
	specs.out_info = NULL;

	if (!sp_set_specs(&specs))
	{
		fprintf(stderr, "can't set specs\n");
	} else
	{
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id)
			{
				printf("/* %3d ID %04x */\n", char_index, char_id);
				if (!sp_make_char(char_index))
				{
					fprintf(stderr, "can't make char %d (%x)\n", char_index, char_id);
				}
			}
		}
	}

	fclose(fp);

	return 0;
}


#if INCL_LCD
boolean sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		fprintf(stderr, "can't seek to char\n");
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		fprintf(stderr, "char buf overflow\n");
		return FALSE;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		fprintf(stderr, "can't get char data\n");
		return FALSE;
	}
	char_data->org = (ufix8 *) c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif


void sp_write_error(const char *str, ...)
{
	va_list v;

	va_start(v, str);
	vfprintf(stderr, str, v);
	va_end(v);
	fputc('\n', stderr);
}


void sp_open_bitmap(fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	UNUSED(xorg);
	UNUSED(yorg);
	UNUSED(xsize);
	UNUSED(ysize);
}


void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	UNUSED(y);
	UNUSED(xbit1);
	UNUSED(xbit2);
}


void sp_close_bitmap(void)
{
}

/* outline stubs */
#if INCL_OUTLINE
void sp_open_outline(fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	UNUSED(x_set_width);
	UNUSED(y_set_width);
	UNUSED(xmin);
	UNUSED(xmax);
	UNUSED(ymin);
	UNUSED(ymax);
}

void sp_start_new_char(void)
{
}

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	UNUSED(x);
	UNUSED(y);
	UNUSED(outside);
}

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	UNUSED(x1);
	UNUSED(y1);
	UNUSED(x2);
	UNUSED(y2);
	UNUSED(x3);
	UNUSED(y3);
}

void sp_line_to(fix31 x, fix31 y)
{
	UNUSED(x);
	UNUSED(y);
}

void sp_close_contour(void)
{
}

void sp_close_outline(void)
{
}
#endif
