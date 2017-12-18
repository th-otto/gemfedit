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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "speedo.h"
#include "iso8859.h"

#ifdef EXTRAFONTS
#include	"ncdkeys.h"
#else
#include	"keys.h"
#endif

#define	MAX_BITS	1024

#define	BBOX_CLIP

static char line_of_bits[MAX_BITS][MAX_BITS + 1];

static FILE *fp;

static ufix16 char_index, char_id;

static buff_t font;

static buff_t char_data;

static ufix8 *f_buffer;
static ufix8 *c_buffer;

static ufix16 mincharsize;

static fix15 cur_y;

static fix15 bit_width, bit_height;

static ufix8 key[] = {
	KEY0,
	KEY1,
	KEY2,
	KEY3,
	KEY4,
	KEY5,
	KEY6,
	KEY7,
	KEY8
};										/* Font decryption key */


static char *progname;

static char *fontname = NULL;

static char *fontfile = NULL;

static int point_size = 120;

static int x_res = 72;

static int y_res = 72;

static int quality = 0;

static int iso_encoding = 0;

static int stretch = 120;

static specs_t specs;

static void usage(void)
{
	fprintf(stderr,
			"Usage: %s [-xres x resolution] [-yres y resolution]\n\t[-ptsize pointsize] [-fn fontname] [-q quality (0-1)] fontfile\n",
			progname);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, "-xres specifies the X resolution (72)\n");
	fprintf(stderr, "-yres specifies the Y resolution (72)\n");
	fprintf(stderr, "-pts specifies the pointsize in decipoints (120)\n");
	fprintf(stderr, "-fn specifies the font name (full Bitstream name)\n");
	fprintf(stderr, "-q specifies the font quality [0-1] (0)\n");
	fprintf(stderr, "\n");
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
		if (!strncmp(av[i], "-xr", 3))
		{
			if (av[i + 1])
			{
				x_res = atoi(av[++i]);
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-yr", 3))
		{
			if (av[i + 1])
			{
				y_res = atoi(av[++i]);
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-pt", 3))
		{
			if (av[i + 1])
			{
				point_size = atoi(av[++i]);
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-fn", 3))
		{
			if (av[i + 1])
			{
				fontname = av[++i];
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-q", 2))
		{
			if (av[i + 1])
			{
				quality = atoi(av[++i]);
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-st", 3))
		{
			if (av[i + 1])
			{
				stretch = atoi(av[++i]);
			} else
			{
				usage();
			}
		} else if (!strncmp(av[i], "-noni", 5))
		{
			iso_encoding = 0;
		} else if (!strncmp(av[i], "-iso", 4))
		{
			iso_encoding = 1;
		} else if (*av[i] == '-')
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


static void dump_header(ufix32 num_chars)
{
	fix15 xmin, ymin, xmax, ymax;
	fix15 ascent, descent;
	fix15 pixel_size;
	const char *weight;
	const char *width;
	
	xmin = read_2b(f_buffer + FH_FXMIN);
	ymin = read_2b(f_buffer + FH_FYMIN);
	xmax = read_2b(f_buffer + FH_FXMAX);
	ymax = read_2b(f_buffer + FH_FYMAX);
	pixel_size = point_size * x_res / 720;

	printf("STARTFONT 2.1\n");
	printf("COMMENT\n");
	printf("COMMENT Generated from Bitstream Speedo outlines via sptobdf\n");
	printf("COMMENT\n");
	printf("FONT %s\n", fontname);
	printf("SIZE %d %d %d\n", pixel_size, x_res, y_res);
	printf("FONTBOUNDINGBOX %d %d %d %d\n", xmin, ymin, xmax, ymax);
	printf("STARTPROPERTIES %d\n", 10);

	printf("RESOLUTION_X %d\n", x_res);
	printf("RESOLUTION_Y %d\n", y_res);
	printf("POINT_SIZE %d\n", point_size);
	printf("PIXEL_SIZE %d\n", pixel_size);
	printf("COPYRIGHT \"%s\"\n", f_buffer + FH_CPYRT);
	printf("FACE_NAME \"%.16s\"\n", f_buffer + FH_SFACN);
	switch (f_buffer[FH_FRMCL] & 0x0f)
	{
		case 4: width = "condensed"; break;
		case 6: width = "semi-condensed"; break;
		case 8: width = "normal"; break;
		case 10: width = "semi-expanded"; break;
		case 12: width = "expanded"; break;
		default: width = ""; break;
	}
	switch ((f_buffer[FH_FRMCL] >> 4) & 0x0f)
	{
		case 1: weight = "thin"; break;
		case 2: weight = "ultralight"; break;
		case 3: weight = "extralight"; break;
		case 4: weight = "light"; break;
		case 5: weight = "book"; break;
		case 6: weight = "normal"; break;
		case 7: weight = "medium"; break;
		case 8: weight = "semibold"; break;
		case 9: weight = "demibold"; break;
		case 10: weight = "bold"; break;
		case 11: weight = "extrabold"; break;
		case 12: weight = "ultrabold"; break;
		case 13: weight = "heavy"; break;
		case 143: weight = "black"; break;
		default: weight = ""; break;
	}
	printf("WIDTH_NAME \"%s\"\n", width);
	printf("WEIGHT_NAME \"%s\"\n", weight);
	
	/* do some stretching here so that its isn't too tight */
	pixel_size = pixel_size * stretch / 100;
	ascent = pixel_size * 764 / 1000;	/* 764 == EM_TOP */
	descent = pixel_size - ascent;
	printf("FONT_ASCENT %d\n", ascent);
	printf("FONT_DESCENT %d\n", descent);

	printf("ENDPROPERTIES\n");
	printf("CHARS %d\n", num_chars);
}


int main(int argc, char **argv)
{
	ufix32 i;
	ufix8 tmp[16];
	ufix32 minbufsize;
	ufix16 cust_no;
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
	fseek(fp, (ufix32) 0, 0);

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

	if ((cust_no = sp_get_cust_no(font)) != CUS0)
	{
#if 0
		fprintf(stderr, "Non-standard encryption for \"%s\"\n", fontfile);
		return 1;
#endif
	}
	sp_set_key(key);

	first_char_index = read_2b(f_buffer + FH_FCHRF);
	num_chars = read_2b(f_buffer + FH_NCHRL);

	/* set up specs */
	/* Note that point size is in decipoints */
	specs.pfont = &font;
	/* XXX beware of overflow */
	specs.xxmult = point_size * x_res / 720 * (1 << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1 << 16);
	specs.yoffset = 0L << 16;
	switch (quality)
	{
	case 0:
		specs.flags = 0;
		break;
	case 1:
		specs.flags = MODE_SCREEN;
		break;
	case 2:
		specs.flags = MODE_2D;
		break;
	default:
		fprintf(stderr, "bogus quality value %d\n", quality);
		break;
	}
	specs.out_info = NULL;

	if (!fontname)
	{
		fontname = (char *) (f_buffer + FH_FNTNM);
	}
	if (iso_encoding)
		num_chars = num_iso_chars;
	dump_header(num_chars);

	if (!sp_set_specs(&specs))
	{
		fprintf(stderr, "can't set specs\n");
	} else
	{
		if (iso_encoding)
		{
			for (i = 0; i < num_iso_chars * 2; i += 2)
			{
				char_index = iso_map[i + 1];
				char_id = iso_map[i];
				if (!sp_make_char(char_index))
				{
					fprintf(stderr, "can't make char %d (%x)\n", char_index, char_id);
				}
			}
		} else
		{
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(char_index);
				if (char_id)
				{
					if (!sp_make_char(char_index))
					{
						fprintf(stderr, "can't make char %d (%x)\n", char_index, char_id);
					}
				}
			}
		}
	}

	(void) fclose(fp);

	printf("ENDFONT\n");
	
	return 0;
}


buff_t *sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset)
{
	if (fseek(fp, (long) file_offset, (int) 0))
	{
		fprintf(stderr, "can't seek to char\n");
		(void) fclose(fp);
		exit(1);
	}
	if ((num + cb_offset) > mincharsize)
	{
		fprintf(stderr, "char buf overflow\n");
		(void) fclose(fp);
		exit(2);
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		fprintf(stderr, "can't get char data\n");
		exit(1);
	}
	char_data.org = (ufix8 *) c_buffer + cb_offset;
	char_data.no_bytes = num;

	return &char_data;
}


/*
 * Called by Speedo character generator to report an error.
 *
 *  Since character data not available is one of those errors
 *  that happens many times, don't report it to user
 */
void sp_report_error(fix15 n)
{
	switch (n)
	{
	case 1:
		fprintf(stderr, "Insufficient font data loaded\n");
		break;
	case 3:
		fprintf(stderr, "Transformation matrix out of range\n");
		break;
	case 4:
		fprintf(stderr, "Font format error\n");
		break;
	case 5:
		fprintf(stderr, "Requested specs not compatible with output module\n");
		break;
	case 7:
		fprintf(stderr, "Intelligent transformation requested but not supported\n");
		break;
	case 8:
		fprintf(stderr, "Unsupported output mode requested\n");
		break;
	case 9:
		fprintf(stderr, "Extended font loaded but only compact fonts supported\n");
		break;
	case 10:
		fprintf(stderr, "Font specs not set prior to use of font\n");
		break;
	case 12:
		break;
	case 13:
		fprintf(stderr, "Track kerning data not available()\n");
		break;
	case 14:
		fprintf(stderr, "Pair kerning data not available()\n");
		break;
	default:
		fprintf(stderr, "report_error(%d)\n", n);
		break;
	}
}


void sp_open_bitmap(fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	fix15 i, y;
	fix15 off_horz;
	fix15 off_vert;
	fix31 width, pix_width;
	bbox_t bb;

	bit_width = xsize;
	bit_height = ysize;

	off_horz = (fix15) ((xorg + 32768L) >> 16);
	off_vert = (fix15) ((yorg + 32768L) >> 16);

	if (bit_width > MAX_BITS)
	{
		fprintf(stderr, "char 0x%x (0x%x) wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	width = sp_get_char_width(char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 7200L) / (point_size * y_res);

	(void) sp_get_char_bbox(char_index, &bb);
	bb.xmin >>= 16;
	bb.ymin >>= 16;
	bb.xmax >>= 16;
	bb.ymax >>= 16;

#ifdef DEBUG
	if ((bb.xmax - bb.xmin) != bit_width)
		fprintf(stderr, "bbox & width mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.xmax - bb.xmin), bit_width);
	if ((bb.ymax - bb.ymin) != bit_height)
		fprintf(stderr, "bbox & height mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.ymax - bb.ymin), bit_height);
	if (bb.xmin != off_horz)
		fprintf(stderr, "x min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.xmin, off_horz);
	if (bb.ymin != off_vert)
		fprintf(stderr, "y min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.ymin, off_vert);
#endif

#ifdef BBOX_CLIP
	bit_width = bb.xmax - bb.xmin;
	bit_height = bb.ymax - bb.ymin;
	off_horz = bb.xmin;
	off_vert = bb.ymin;
#endif

	/* XXX kludge to handle space */
	if (bb.xmin == 0 && bb.ymin == 0 && bb.xmax == 0 && bb.ymax == 0 && width)
	{
		bit_width = 1;
		bit_height = 1;
	}
	printf("STARTCHAR %d\n", char_index);
	printf("ENCODING %d\n", char_id);
	printf("SWIDTH %d 0\n", width);
	printf("DWIDTH %d 0\n", pix_width);
	printf("BBX %d %d %d %d\n", bit_width, bit_height, off_horz, off_vert);
	printf("BITMAP\n");

	if (bit_width > MAX_BITS)
	{
		fprintf(stderr, "width too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		fprintf(stderr, "height too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_height = MAX_BITS;
	}
	
	for (y = 0; y < bit_height; y++)
	{
		for (i = 0; i < bit_width; i++)
		{
			line_of_bits[y][i] = ' ';
		}
		line_of_bits[y][bit_width] = '\0';
	}
	cur_y = 0;
}


static void dump_line(const char *line)
{
	int bit;
	unsigned byte;

	byte = 0;
	for (bit = 0; bit < bit_width; bit++)
	{
		if (line[bit] != ' ')
			byte |= (1 << (7 - (bit & 7)));
		if ((bit & 7) == 7)
		{
			printf("%02X", byte);
			byte = 0;
		}
	}
	if ((bit & 7) != 0)
		printf("%02X", byte);
	printf("\n");
}

#ifdef BBOX_CLIP
static fix15 last_y;

static int trunc = 0;

#endif


void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 > MAX_BITS)
	{

#ifdef DEBUG
		fprintf(stderr, "run wider than max bits -- truncated\n");
#endif

		xbit1 = MAX_BITS;
	}
	if (xbit2 > MAX_BITS)
	{

#ifdef DEBUG
		fprintf(stderr, "run wider than max bits -- truncated\n");
#endif

		xbit2 = MAX_BITS;
	}
	cur_y = y;

#ifdef BBOX_CLIP
	last_y = y;
	if (y >= bit_height)
	{
#ifdef DEBUG
		fprintf(stderr, "y value is larger than height 0x%x (0x%x) -- truncated\n", char_index, char_id);
#endif

		trunc = 1;
		return;
	}
#endif /* BBOX_CLIP */

	for (i = xbit1; i < xbit2; i++)
	{
		line_of_bits[y][i] = '*';
	}
}


void sp_close_bitmap(void)
{
	int y, i;
#ifdef BBOX_CLIP

	trunc = 0;

	last_y++;
	while (last_y < bit_height)
	{
#ifdef DEBUG
		fprintf(stderr, "padding out height for 0x%x (0x%x)\n", char_index, char_id);
#endif
		last_y++;
	}

#endif

	for (y = 0; y < bit_height; y++)
	{
		printf("COMMENT %s\n", line_of_bits[y]);
	}
	
	for (y = 0; y < bit_height; y++)
		dump_line(line_of_bits[y]);

	printf("ENDCHAR\n");

	for (y = 0; y < bit_height; y++)
	{
		for (i = 0; i < bit_width; i++)
		{
			line_of_bits[y][i] = ' ';
		}
		line_of_bits[y][bit_width] = '\0';
	}
}

/* outline stubs */
#if INCL_OUTLINE
void sp_open_outline(fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	(void) x_set_width;
	(void) y_set_width;
	(void) xmin;
	(void) xmax;
	(void) ymin;
	(void) ymax;
}

void sp_start_new_char(void)
{
}

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	(void) x;
	(void) y;
	(void) outside;
}

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	(void) x1;
	(void) y1;
	(void) x2;
	(void) y2;
	(void) x3;
	(void) y3;
}

void sp_line_to(fix31 x, fix31 y)
{
	(void) x;
	(void) y;
}

void sp_close_contour(void)
{
}

void sp_close_outline(void)
{
}
#endif
