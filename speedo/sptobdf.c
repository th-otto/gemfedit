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
#include "spdo_prv.h"
#include "iso8859.h"

static char const progname[] = "spdtobdf";

#define	MAX_BITS	1024

static char line_of_bits[MAX_BITS][MAX_BITS + 1];
static ufix16 char_index, char_id;
static buff_t font;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static ufix16 mincharsize;
static fix15 bit_width, bit_height;
static char *fontname = NULL;
static char *fontfile = NULL;

static long point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 0;
static int iso_encoding = 0;
static int stretch = 120;

static specs_t specs;

#define DEBUG 0

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


/*
 * Reads 1-byte field from font buffer 
 */
static ufix8 read_1b(ufix8 *pointer)
{
	return *pointer;
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
				point_size = strtol(av[++i], NULL, 10);
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
	long pixel_size;
	const char *weight;
	const char *width;
	
	xmin = read_2b(font_buffer + FH_FXMIN);
	ymin = read_2b(font_buffer + FH_FYMIN);
	xmax = read_2b(font_buffer + FH_FXMAX);
	ymax = read_2b(font_buffer + FH_FYMAX);
	pixel_size = point_size * x_res / 720;

	printf("STARTFONT 2.1\n");
	printf("COMMENT\n");
	printf("COMMENT Generated from Bitstream Speedo outlines via sptobdf\n");
	printf("COMMENT\n");
	printf("COMMENT Font size: %ld\n", (long)read_4b(font_buffer + FH_FNTSZ));
	printf("COMMENT Minimum font buffer size: %ld\n", (long)read_4b(font_buffer + FH_FBFSZ));
	printf("COMMENT Minimum character buffer size: %u\n", read_2b(font_buffer + FH_CBFSZ));
	printf("COMMENT Header size: %u\n", read_2b(font_buffer + FH_HEDSZ));
	printf("COMMENT Font ID: %u\n", read_2b(font_buffer + FH_FNTID));
	printf("COMMENT Font version number: %u\n", read_2b(font_buffer + FH_SFVNR));
	printf("COMMENT Font full name: %.70s\n", font_buffer + FH_FNTNM);
	printf("COMMENT Short font name: %.32s\n", font_buffer + FH_SFNTN);
	printf("COMMENT Short face name: %.16s\n", font_buffer + FH_SFACN);
	printf("COMMENT Font form: %.14s\n", font_buffer + FH_FNTFM);
	printf("COMMENT Manufacturing date: %.10s\n", font_buffer + FH_MDATE);
	printf("COMMENT Layout name: %.70s\n", font_buffer + FH_LAYNM);
	printf("COMMENT Number of chars in layout: %u\n", read_2b(font_buffer + FH_NCHRL));
	printf("COMMENT Total Number of chars in font: %u\n", read_2b(font_buffer + FH_NCHRF));
	printf("COMMENT Index of first character: %u\n", read_2b(font_buffer + FH_FCHRF));
	printf("COMMENT Number of Kerning Tracks: %u\n", read_2b(font_buffer + FH_NKTKS));
	printf("COMMENT Number of Kerning Pairs: %u\n", read_2b(font_buffer + FH_NKPRS));
	printf("COMMENT Flags: 0x%x\n", read_1b(font_buffer + FH_FLAGS));
	printf("COMMENT Classification Flags: 0x%x\n", read_1b(font_buffer + FH_CLFGS));
	printf("COMMENT Family Classification: 0x%x\n", read_1b(font_buffer + FH_FAMCL));
	printf("COMMENT Font form classification: 0x%x\n", read_1b(font_buffer + FH_FRMCL));
	printf("COMMENT Italic angle: %d\n", read_2b(font_buffer + FH_ITANG));
	printf("COMMENT Number of ORUs per em: %d\n", read_2b(font_buffer + FH_ORUPM));
	printf("COMMENT Width of Wordspace: %d\n", read_2b(font_buffer + FH_WDWTH));
	printf("COMMENT Width of Emspace: %d\n", read_2b(font_buffer + FH_EMWTH));
	printf("COMMENT Width of Enspace: %d\n", read_2b(font_buffer + FH_ENWTH));
	printf("COMMENT Width of Thinspace: %d\n", read_2b(font_buffer + FH_TNWTH));
	printf("COMMENT Width of Figspace: %d\n", read_2b(font_buffer + FH_FGWTH));
	printf("COMMENT Font-wide min X value: %d\n", read_2b(font_buffer + FH_FXMIN));
	printf("COMMENT Font-wide min Y value: %d\n", read_2b(font_buffer + FH_FYMIN));
	printf("COMMENT Font-wide max X value: %d\n", read_2b(font_buffer + FH_FXMAX));
	printf("COMMENT Font-wide max Y value: %d\n", read_2b(font_buffer + FH_FYMAX));
	printf("COMMENT Underline position: %d\n", read_2b(font_buffer + FH_ULPOS));
	printf("COMMENT Underline thickness: %d\n", read_2b(font_buffer + FH_ULTHK));
	printf("COMMENT Small caps: %d\n", read_2b(font_buffer + FH_SMCTR));
	printf("COMMENT Display Superiors Y position: %d\n", read_2b(font_buffer + FH_DPSTR));
	printf("COMMENT Display Superiors X scale: %7.2f\n", (real) read_2b(font_buffer + FH_DPSTR + 2) / 4096.0);
	printf("COMMENT Display Superiors Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_DPSTR + 4) / 4096.0);
	printf("COMMENT Footnote Superiors Y position: %d\n", read_2b(font_buffer + FH_FNSTR));
	printf("COMMENT Footnote Superiors X scale: %7.2f\n", (real) read_2b(font_buffer + FH_FNSTR + 2) / 4096.0);
	printf("COMMENT Footnote Superiors Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_FNSTR + 4) / 4096.0);
	printf("COMMENT Alpha Superiors Y position: %d\n", read_2b(font_buffer + FH_ALSTR));
	printf("COMMENT Alpha Superiors X scale: %7.2f\n", (real) read_2b(font_buffer + FH_ALSTR + 2) / 4096.0);
	printf("COMMENT Alpha Superiors Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_ALSTR + 4) / 4096.0);
	printf("COMMENT Chemical Inferiors Y position: %d\n", read_2b(font_buffer + FH_CMITR));
	printf("COMMENT Chemical Inferiors X scale: %7.2f\n", (real) read_2b(font_buffer + FH_CMITR + 2) / 4096.0);
	printf("COMMENT Chemical Inferiors Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_CMITR + 4) / 4096.0);
	printf("COMMENT Small Numerators Y position: %d\n", read_2b(font_buffer + FH_SNMTR));
	printf("COMMENT Small Numerators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_SNMTR + 2) / 4096.0);
	printf("COMMENT Small Numerators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_SNMTR + 4) / 4096.0);
	printf("COMMENT Small Denominators Y position: %d\n", read_2b(font_buffer + FH_SDNTR));
	printf("COMMENT Small Denominators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_SDNTR + 2) / 4096.0);
	printf("COMMENT Small Denominators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_SDNTR + 4) / 4096.0);
	printf("COMMENT Medium Numerators Y position: %d\n", read_2b(font_buffer + FH_MNMTR));
	printf("COMMENT Medium Numerators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_MNMTR + 2) / 4096.0);
	printf("COMMENT Medium Numerators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_MNMTR + 4) / 4096.0);
	printf("COMMENT Medium Denominators Y position: %d\n", read_2b(font_buffer + FH_MDNTR));
	printf("COMMENT Medium Denominators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_MDNTR + 2) / 4096.0);
	printf("COMMENT Medium Denominators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_MDNTR + 4) / 4096.0);
	printf("COMMENT Large Numerators Y position: %d\n", read_2b(font_buffer + FH_LNMTR));
	printf("COMMENT Large Numerators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_LNMTR + 2) / 4096.0);
	printf("COMMENT Large Numerators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_LNMTR + 4) / 4096.0);
	printf("COMMENT Large Denominators Y position: %d\n", read_2b(font_buffer + FH_LDNTR));
	printf("COMMENT Large Denominators X scale: %7.2f\n", (real) read_2b(font_buffer + FH_LDNTR + 2) / 4096.0);
	printf("COMMENT Large Denominators Y scale: %7.2f\n", (real) read_2b(font_buffer + FH_LDNTR + 4) / 4096.0);
	printf("COMMENT\n");

	{
		ufix8 *hdr2_org;
		ufix16 private_off;

		private_off = sp_read_word_u(font.org + FH_HEDSZ);
		if (private_off + FH_CUSNR > font.no_bytes)
		{
			sp_report_error(1);				/* Insufficient font data loaded */
		} else
		{
			hdr2_org = font.org + private_off;

			printf("COMMENT Max ORU value: %u\n", sp_read_word_u(hdr2_org + FH_ORUMX));
			printf("COMMENT Max Pixel value: %u\n", sp_read_word_u(hdr2_org + FH_PIXMX));
			printf("COMMENT Customer Number: %u\n", sp_read_word_u(hdr2_org + FH_CUSNR));
			printf("COMMENT Offset to Char Directory: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_OFFCD));
			printf("COMMENT Offset to Constraint Data: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_OFCNS));
			printf("COMMENT Offset to Track Kerning: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_OFFTK));
			printf("COMMENT Offset to Pair Kerning: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_OFFPK));
			printf("COMMENT Offset to Character Data: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_OCHRD));
			printf("COMMENT Number of Bytes in File: %lu\n", (unsigned long)sp_read_long(hdr2_org + FH_NBYTE));
		}
		printf("COMMENT\n");
	}
	
	printf("FONT %s\n", fontname);
	printf("SIZE %ld %d %d\n", pixel_size, x_res, y_res);
	printf("FONTBOUNDINGBOX %d %d %d %d\n", xmin, ymin, xmax, ymax);
	printf("STARTPROPERTIES %d\n", 10);

	printf("RESOLUTION_X %d\n", x_res);
	printf("RESOLUTION_Y %d\n", y_res);
	printf("POINT_SIZE %ld\n", point_size);
	printf("PIXEL_SIZE %ld\n", pixel_size);
	printf("COPYRIGHT \"%.78s\"\n", font_buffer + FH_CPYRT);
	printf("FACE_NAME \"%.16s\"\n", font_buffer + FH_SFACN);
	switch (font_buffer[FH_FRMCL] & 0x0f)
	{
		case 4: width = "condensed"; break;
		case 6: width = "semi-condensed"; break;
		case 8: width = "normal"; break;
		case 10: width = "semi-expanded"; break;
		case 12: width = "expanded"; break;
		default: width = ""; break;
	}
	switch ((font_buffer[FH_FRMCL] >> 4) & 0x0f)
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


static FILE *fp;

int main(int argc, char **argv)
{
	ufix32 i;
	ufix8 tmp[16];
	ufix32 minbufsize;
	int first_char_index, num_chars, real_num_chars;
	const ufix8 *key;
	
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
	font_buffer = (ufix8 *) malloc(minbufsize);
	if (font_buffer == NULL)
	{
		fprintf(stderr, "can't get %x bytes of memory\n", minbufsize);
		return 1;
	}
	fseek(fp, 0, SEEK_SET);

	if (fread(font_buffer, sizeof(ufix8), minbufsize, fp) != minbufsize)
	{
		fprintf(stderr, "error reading file \"%s\"\n", fontfile);
		return 1;
	}
	mincharsize = read_2b(font_buffer + FH_CBFSZ);

	c_buffer = (ufix8 *) malloc(mincharsize);
	if (!c_buffer)
	{
		fprintf(stderr, "can't get %x bytes for char buffer\n", mincharsize);
		return 1;
	}
	/* init */
	sp_reset();

	font.org = font_buffer;
	font.no_bytes = minbufsize;

	key = sp_get_key(&font);
	if (key == NULL)
	{
		fprintf(stderr, "Non-standard encryption for \"%s\"\n", fontfile);
#if 0
		return 1;
#endif
	} else
	{
		sp_set_key(key);
	}
	
	first_char_index = read_2b(font_buffer + FH_FCHRF);
	num_chars = read_2b(font_buffer + FH_NCHRL);

	/* set up specs */
	/* Note that point size is in decipoints */
	specs.pfont = &font;
	/* XXX beware of overflow */
	specs.xxmult = point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1L << 16);
	specs.yoffset = 0L << 16;
	switch (quality)
	{
	case 0:
		specs.flags = MODE_BLACK;
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
		fontname = (char *) (font_buffer + FH_FNTNM);
	}
	if (!sp_set_specs(&specs))
	{
		fprintf(stderr, "can't set specs\n");
	} else
	{
		if (iso_encoding)
		{
			num_chars = num_iso_chars;
			real_num_chars = 0;
			for (i = 0; i < num_iso_chars * 2; i += 2)
			{
				char_index = iso_map[i + 1];
				char_id = iso_map[i];
				real_num_chars++;
			}
		} else
		{
			real_num_chars = 0;
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(char_index);
				if (char_id)
					real_num_chars++;
			}
		}
		dump_header(real_num_chars);

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


#if INCL_LCD
boolean sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		fprintf(stderr, "can't seek to char\n");
		(void) fclose(fp);
		exit(1);
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		fprintf(stderr, "char buf overflow\n");
		(void) fclose(fp);
		exit(2);
		return FALSE;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		fprintf(stderr, "can't get char data\n");
		exit(1);
		return FALSE;
	}
	char_data->org = (ufix8 *) c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif


/*
 * Called by Speedo character generator to report an error.
 */
void sp_write_error(const char *str, ...)
{
	va_list v;

	va_start(v, str);
	vfprintf(stderr, str, v);
	va_end(v);
	fputc('\n', stderr);
}


void sp_open_bitmap(fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	fix15 i, y;
	fix15 off_horz;
	fix15 off_vert;
	fix31 width, pix_width;
	bbox_t bb;

	UNUSED(x_set_width);
	UNUSED(y_set_width);
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

	sp_get_char_bbox(char_index, &bb);
	bb.xmin >>= 16;
	bb.ymin >>= 16;
	bb.xmax >>= 16;
	bb.ymax >>= 16;

#if DEBUG
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

	bit_width = bb.xmax - bb.xmin;
	bit_height = bb.ymax - bb.ymin;
	off_horz = bb.xmin;
	off_vert = bb.ymin;

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

static int trunc = 0;


void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 >= MAX_BITS)
	{
#if DEBUG
		fprintf(stderr, "run wider than max bits -- truncated\n");
#endif

		xbit1 = MAX_BITS - 1;
	}
	if (xbit2 > MAX_BITS)
	{

#if DEBUG
		fprintf(stderr, "run wider than max bits -- truncated\n");
#endif

		xbit2 = MAX_BITS;
	}

	if (y >= bit_height)
	{
#if DEBUG
		fprintf(stderr, "y value is larger than height 0x%x (0x%x) -- truncated\n", char_index, char_id);
#endif

		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		line_of_bits[y][i] = '*';
	}
}


void sp_close_bitmap(void)
{
	int y, i;

	trunc = 0;

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

void sp_line_to(fix31 x1, fix31 y1)
{
	UNUSED(x1);
	UNUSED(y1);
}

void sp_close_contour(void)
{
}

void sp_close_outline(void)
{
}
#endif
