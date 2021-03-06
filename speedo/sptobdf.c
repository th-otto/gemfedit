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

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	glyphinfo_t bbox;
} charinfo;

static specs_t specs;

#define DEBUG 0

static void usage(void)
{
	fprintf(stderr,
			"Usage: %s [-xres x resolution] [-yres y resolution]\n\t[-ptsize pointsize] [-fn fontname] [-q quality (0-1)] fontfile\n",
			progname);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, "-xres specifies the X resolution (%d)\n", x_res);
	fprintf(stderr, "-yres specifies the Y resolution (%d)\n", y_res);
	fprintf(stderr, "-pts specifies the pointsize in decipoints (%ld)\n", point_size);
	fprintf(stderr, "-fn specifies the font name (full Bitstream name)\n");
	fprintf(stderr, "-q specifies the font quality [0-3] (%d)\n", quality);
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


static void dump_header(SPD_PROTO_DECL2 uint16_t num_chars, const glyphinfo_t *box, boolean is_mono)
{
	long pixel_size;
	const char *weight;
	const char *width;
	
	pixel_size = (point_size * x_res + 360) / 720;

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
	printf("COMMENT Display Superiors X scale: %7.2f\n", (double) read_2b(font_buffer + FH_DPSTR + 2) / 4096.0);
	printf("COMMENT Display Superiors Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_DPSTR + 4) / 4096.0);
	printf("COMMENT Footnote Superiors Y position: %d\n", read_2b(font_buffer + FH_FNSTR));
	printf("COMMENT Footnote Superiors X scale: %7.2f\n", (double) read_2b(font_buffer + FH_FNSTR + 2) / 4096.0);
	printf("COMMENT Footnote Superiors Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_FNSTR + 4) / 4096.0);
	printf("COMMENT Alpha Superiors Y position: %d\n", read_2b(font_buffer + FH_ALSTR));
	printf("COMMENT Alpha Superiors X scale: %7.2f\n", (double) read_2b(font_buffer + FH_ALSTR + 2) / 4096.0);
	printf("COMMENT Alpha Superiors Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_ALSTR + 4) / 4096.0);
	printf("COMMENT Chemical Inferiors Y position: %d\n", read_2b(font_buffer + FH_CMITR));
	printf("COMMENT Chemical Inferiors X scale: %7.2f\n", (double) read_2b(font_buffer + FH_CMITR + 2) / 4096.0);
	printf("COMMENT Chemical Inferiors Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_CMITR + 4) / 4096.0);
	printf("COMMENT Small Numerators Y position: %d\n", read_2b(font_buffer + FH_SNMTR));
	printf("COMMENT Small Numerators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_SNMTR + 2) / 4096.0);
	printf("COMMENT Small Numerators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_SNMTR + 4) / 4096.0);
	printf("COMMENT Small Denominators Y position: %d\n", read_2b(font_buffer + FH_SDNTR));
	printf("COMMENT Small Denominators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_SDNTR + 2) / 4096.0);
	printf("COMMENT Small Denominators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_SDNTR + 4) / 4096.0);
	printf("COMMENT Medium Numerators Y position: %d\n", read_2b(font_buffer + FH_MNMTR));
	printf("COMMENT Medium Numerators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_MNMTR + 2) / 4096.0);
	printf("COMMENT Medium Numerators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_MNMTR + 4) / 4096.0);
	printf("COMMENT Medium Denominators Y position: %d\n", read_2b(font_buffer + FH_MDNTR));
	printf("COMMENT Medium Denominators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_MDNTR + 2) / 4096.0);
	printf("COMMENT Medium Denominators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_MDNTR + 4) / 4096.0);
	printf("COMMENT Large Numerators Y position: %d\n", read_2b(font_buffer + FH_LNMTR));
	printf("COMMENT Large Numerators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_LNMTR + 2) / 4096.0);
	printf("COMMENT Large Numerators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_LNMTR + 4) / 4096.0);
	printf("COMMENT Large Denominators Y position: %d\n", read_2b(font_buffer + FH_LDNTR));
	printf("COMMENT Large Denominators X scale: %7.2f\n", (double) read_2b(font_buffer + FH_LDNTR + 2) / 4096.0);
	printf("COMMENT Large Denominators Y scale: %7.2f\n", (double) read_2b(font_buffer + FH_LDNTR + 4) / 4096.0);
	printf("COMMENT\n");

	{
		ufix8 *hdr2_org;
		ufix16 private_off;

		private_off = sp_read_word_u(SPD_GARGS font.org + FH_HEDSZ);
		if (private_off + FH_CUSNR > font.no_bytes)
		{
			sp_report_error(SPD_GARGS 1);		/* Insufficient font data loaded */
		} else
		{
			hdr2_org = font.org + private_off;

			printf("COMMENT Max ORU value: %u\n", sp_read_word_u(SPD_GARGS hdr2_org + FH_ORUMX));
			printf("COMMENT Max Pixel value: %u\n", sp_read_word_u(SPD_GARGS hdr2_org + FH_PIXMX));
			printf("COMMENT Customer Number: %u\n", sp_read_word_u(SPD_GARGS hdr2_org + FH_CUSNR));
			printf("COMMENT Offset to Char Directory: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_OFFCD));
			printf("COMMENT Offset to Constraint Data: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_OFCNS));
			printf("COMMENT Offset to Track Kerning: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_OFFTK));
			printf("COMMENT Offset to Pair Kerning: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_OFFPK));
			printf("COMMENT Offset to Character Data: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_OCHRD));
			printf("COMMENT Number of Bytes in File: %lu\n", (unsigned long)sp_read_long(SPD_GARGS hdr2_org + FH_NBYTE));
		}
		printf("COMMENT\n");
	}
	
	printf("FONT %s\n", fontname);
	printf("SIZE %ld %d %d\n", pixel_size, x_res, y_res);

#if 0
	{
		fix15 xmin, ymin, xmax, ymax;
		long fwidth, fheight;
		
		xmin = read_2b(font_buffer + FH_FXMIN);
		ymin = read_2b(font_buffer + FH_FYMIN);
		xmax = read_2b(font_buffer + FH_FXMAX);
		ymax = read_2b(font_buffer + FH_FYMAX);
		fwidth = xmax - xmin;
		fwidth = fwidth * pixel_size / sp_globals.orus_per_em;
		fheight = ymax - ymin;
		fheight = fheight * pixel_size / sp_globals.orus_per_em;
		printf("FONTBOUNDINGBOX %ld %ld %ld %ld\n", fwidth, fheight, xmin * pixel_size / sp_globals.orus_per_em, ymin * pixel_size / sp_globals.orus_per_em);
		UNUSED(box);
	}
#else
	printf("FONTBOUNDINGBOX %d %d %d %d\n", box->rbearing - box->lbearing, box->ascent + box->descent, box->lbearing, -box->descent);
#endif
	
	printf("STARTPROPERTIES %d\n", 14);

	printf("RESOLUTION_X %d\n", x_res);
	printf("RESOLUTION_Y %d\n", y_res);
	printf("POINT_SIZE %ld\n", point_size);
	printf("PIXEL_SIZE %ld\n", pixel_size);
	printf("SPACING \"%s\"\n", is_mono ? "M" : "P");
	printf("SLANT \"%s\"\n", read_1b(font_buffer + FH_CLFGS) & 0x01 ? "I" : "R");
	printf("FONT_TYPE \"%s\"\n", "Speedo");
	printf("RASTERIZER_NAME \"%s\"\n", "X Consortium Speedo Rasterizer");
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
	
#if 0
	{
		fix15 ascent, descent;
		ascent = pixel_size * EM_TOP / 1000;
		descent = pixel_size - ascent;
		printf("FONT_ASCENT %d\n", ascent);
		printf("FONT_DESCENT %d\n", descent);
	}
#else
	printf("FONT_ASCENT %d\n", box->ascent);
	printf("FONT_DESCENT %d\n", box->descent);
#endif

	printf("ENDPROPERTIES\n");
	printf("CHARS %u\n", num_chars);
}


static FILE *fp;

static void update_bbox(SPD_PROTO_DECL2 charinfo *c, glyphinfo_t *box)
{
	bbox_t bb;
	
	sp_get_char_bbox(SPD_GARGS c->char_index, &bb, TRUE);
	c->bbox.xmin = bb.xmin;
	c->bbox.ymin = bb.ymin;
	c->bbox.xmax = bb.xmax;
	c->bbox.ymax = bb.ymax;
	box->xmin = MIN(box->xmin, c->bbox.xmin);
	box->ymin = MIN(box->ymin, c->bbox.ymin);
	box->xmax = MIN(box->xmax, c->bbox.xmax);
	box->ymax = MIN(box->ymax, c->bbox.ymax);
	c->bbox.width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
	c->bbox.height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
	c->bbox.lbearing = bb.xmin >> 16;
	c->bbox.off_vert = bb.ymin >> 16;
	box->lbearing = MIN(box->lbearing, c->bbox.lbearing);
	c->bbox.rbearing = c->bbox.width + c->bbox.lbearing;
	box->rbearing = MAX(box->rbearing, c->bbox.rbearing);
	c->bbox.ascent = c->bbox.height + c->bbox.off_vert;
	c->bbox.descent = c->bbox.height - c->bbox.ascent;
	box->width = MAX(box->width, c->bbox.width);
	box->height = MAX(box->height, c->bbox.height);
	box->ascent = MAX(box->ascent, c->bbox.ascent);
	box->descent = MAX(box->descent, c->bbox.descent);
}


int main(int argc, char **argv)
{
	uint16_t i;
	ufix8 tmp[FH_FBFSZ + 4];
	ufix32 minbufsize;
	uint16_t first_char_index, num_chars, real_num_chars;
	const ufix8 *key;
	
#if !DYNAMIC_ALLOC && !STATIC_ALLOC
	SPEEDO_GLOBALS *sp_global_ptr;
#endif

#if !STATIC_ALLOC
	sp_global_ptr = calloc(1, sizeof(*sp_global_ptr));
#endif

	process_args(argc, argv);
	fp = fopen(fontfile, "rb");
	if (!fp)
	{
		fprintf(stderr, "No such font file, \"%s\"\n", fontfile);
		return 1;
	}
	if (fread(tmp, sizeof(tmp), 1, fp) != 1)
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

	if (fread(font_buffer, minbufsize, 1, fp) != 1)
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
	sp_reset(SPD_GARG);

	font.org = font_buffer;
	font.no_bytes = minbufsize;

	key = sp_get_key(SPD_GARGS &font);
	if (key == NULL)
	{
		fprintf(stderr, "Non-standard encryption for \"%s\"\n", fontfile);
#if 0
		return 1;
#endif
	} else
	{
		sp_set_key(SPD_GARGS key);
	}
	
	first_char_index = read_2b(font_buffer + FH_FCHRF);
	num_chars = read_2b(font_buffer + FH_NCHRL);

	/* set up specs */
	/* Note that point size is in decipoints */
	specs.xxmult = point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1L << 16);
	specs.yoffset = 0L << 16;
	specs.flags = 0;
	switch (quality)
	{
	case 0:
		specs.output_mode = MODE_BLACK;
		break;
	case 1:
		specs.output_mode = MODE_SCREEN;
		break;
	case 2:
#if INCL_OUTLINE
		specs.output_mode = MODE_OUTLINE;
#else
		specs.output_mode = MODE_2D;
#endif
		break;
	case 3:
		specs.output_mode = MODE_2D;
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
	if (!sp_set_specs(SPD_GARGS &specs, &font))
	{
		fprintf(stderr, "can't set specs\n");
	} else
	{
		glyphinfo_t font_bbox;
		charinfo c;
		uint16_t monowidth;
		boolean is_mono;
		
		font_bbox.width = 0;
		font_bbox.height = 0;
		font_bbox.off_vert = 0;
		font_bbox.ascent = -32000;
		font_bbox.descent = -32000;
		font_bbox.rbearing = -32000;
		font_bbox.lbearing = 32000;
		font_bbox.xmin = 32000L << 16;
		font_bbox.ymin = 32000L << 16;
		font_bbox.xmax = -(32000L << 16);
		font_bbox.ymax = -(32000L << 16);
		
		monowidth = 0;
		is_mono = TRUE;
		if (iso_encoding)
		{
			num_chars = num_iso_chars;
			real_num_chars = 0;
			for (i = 0; i < num_iso_chars * 2; i += 2)
			{
				c.char_index = char_index = iso_map[i + 1];
				c.char_id = char_id = iso_map[i];
				if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
				{
					real_num_chars++;
					update_bbox(SPD_GARGS &c, &font_bbox);
					if (c.bbox.width != 0 && c.bbox.width != monowidth)
					{
						if (monowidth != 0)
							is_mono = FALSE;
						monowidth = c.bbox.width;
					}
				}
			}
		} else
		{
			real_num_chars = 0;
			for (i = 0; i < num_chars; i++)
			{
				c.char_index = char_index = i + first_char_index;
				c.char_id = char_id = sp_get_char_id(SPD_GARGS char_index);
				if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
				{
					real_num_chars++;
					update_bbox(SPD_GARGS &c, &font_bbox);
					if (c.bbox.width != 0 && c.bbox.width != monowidth)
					{
						if (monowidth != 0)
							is_mono = FALSE;
						monowidth = c.bbox.width;
					}
				}
			}
		}
		font_bbox.ascent = font_bbox.height - font_bbox.descent;
		dump_header(SPD_GARGS real_num_chars, &font_bbox, is_mono);

		if (iso_encoding)
		{
			for (i = 0; i < num_iso_chars * 2; i += 2)
			{
				char_index = iso_map[i + 1];
				char_id = iso_map[i];
				if (!sp_make_char(SPD_GARGS char_index))
				{
					fprintf(stderr, "can't make char %d (%x)\n", char_index, char_id);
				}
			}
		} else
		{
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(SPD_GARGS char_index);
				if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
				{
					if (!sp_make_char(SPD_GARGS char_index))
					{
						fprintf(stderr, "can't make char %d (%x)\n", char_index, char_id);
					}
				}
			}
		}
	}

	fclose(fp);

	printf("ENDFONT\n");
	
	return 0;
}


#if INCL_LCD
boolean sp_load_char_data(SPD_PROTO_DECL2 long file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	SPD_GUNUSED
	if (fseek(fp, file_offset, SEEK_SET))
	{
		fprintf(stderr, "%x (%x): can't seek to char at %ld\n", char_index, char_id, file_offset);
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		fprintf(stderr, "char buf overflow\n");
		return FALSE;
	}
	if ((long)fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		fprintf(stderr, "can't get char data\n");
		return FALSE;
	}
	char_data->org = c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif


/*
 * Called by Speedo character generator to report an error.
 */
void sp_write_error(SPD_PROTO_DECL2 const char *str, ...)
{
	va_list v;

	SPD_GUNUSED
	va_start(v, str);
	vfprintf(stderr, str, v);
	va_end(v);
	fputc('\n', stderr);
}


void sp_open_bitmap(SPD_PROTO_DECL2 fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
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
	width = sp_get_char_width(SPD_GARGS char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 720000L) / (point_size * x_res);

	sp_get_char_bbox(SPD_GARGS char_index, &bb, TRUE);

#if DEBUG
	if (((bb.xmax - bb.xmin) >> 16) != bit_width)
		fprintf(stderr, "bbox & width mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.xmax - bb.xmin) >> 16, bit_width);
	if (((bb.ymax - bb.ymin) >> 16) != bit_height)
		fprintf(stderr, "bbox & height mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.ymax - bb.ymin) >> 16, bit_height);
	if ((bb.xmin >> 16) != off_horz)
		fprintf(stderr, "x min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.xmin >> 16, off_horz);
	if ((bb.ymin >> 16) != off_vert)
		fprintf(stderr, "y min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.ymin >> 16, off_vert);
#endif

	bit_width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
	bit_height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
	off_horz = bb.xmin >> 16;
	off_vert = bb.ymin >> 16;

	/* XXX kludge to handle space */
	if (bb.xmin == 0 && bb.ymin == 0 && bb.xmax == 0 && bb.ymax == 0 && width)
	{
		bit_width = 1;
		bit_height = 1;
	}

	printf("STARTCHAR %d\n", char_index);
	printf("ENCODING %d\n", char_id);
	printf("SWIDTH %ld 0\n", (long)width);
	printf("DWIDTH %ld 0\n", (long)pix_width);
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


void sp_set_bitmap_bits(SPD_PROTO_DECL2 fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	SPD_GUNUSED
	if (xbit1 < 0 || xbit1 >= MAX_BITS)
	{
		fprintf(stderr, "char 0x%x (0x%x): bit1 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit1, bit_width);
		xbit1 = MAX_BITS - 1;
		trunc = 1;
	}
	if (xbit2 < 0 || xbit2 > MAX_BITS)
	{
		fprintf(stderr, "char 0x%x (0x%x): bit2 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit2, bit_width);
		xbit2 = MAX_BITS;
		trunc = 1;
	}

	if (y >= bit_height)
	{
		fprintf(stderr, "char 0x%x (0x%x): y value %d is larger than height %u -- truncated\n", char_index, char_id, y, bit_height);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		line_of_bits[y][i] = '*';
	}
}


void sp_close_bitmap(SPD_PROTO_DECL1)
{
	int y, i;

	SPD_GUNUSED
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
void sp_open_outline(SPD_PROTO_DECL2 fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	SPD_GUNUSED
	printf("\n");
	printf("/* char_index: %d id: %04x */\n", char_index, char_id);
	printf("open_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x_set_width / 65536.0, (double) y_set_width / 65536.0,
		   (double) xmin / 65536.0, (double) xmax / 65536.0,
		   (double) ymin / 65536.0, (double) ymax / 65536.0);
}

void sp_start_sub_char(SPD_PROTO_DECL1)
{
	SPD_GUNUSED
	printf("start_sub_char()\n");
}

void sp_end_sub_char(SPD_PROTO_DECL1)
{
	SPD_GUNUSED
	printf("end_sub_char()\n");
}

void sp_start_contour(SPD_PROTO_DECL2 fix31 x, fix31 y, boolean outside)
{
	SPD_GUNUSED
	printf("start_contour(%3.1f, %3.1f, %s)\n", (double) x / 65536.0, (double) y / 65536.0, outside ? "outside" : "inside");
}

void sp_curve_to(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	SPD_GUNUSED
	printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x1 / 65536.0, (double) y1 / 65536.0,
		   (double) x2 / 65536.0, (double) y2 / 65536.0,
		   (double) x3 / 65536.0, (double) y3 / 65536.0);
}

void sp_line_to(SPD_PROTO_DECL2 fix31 x1, fix31 y1)
{
	SPD_GUNUSED
	printf("line_to(%3.1f, %3.1f)\n", (double) x1 / 65536.0, (double) y1 / 65536.0);
}

void sp_close_contour(SPD_PROTO_DECL1)
{
	SPD_GUNUSED
	printf("close_contour()\n");
}

void sp_close_outline(SPD_PROTO_DECL1)
{
	SPD_GUNUSED
	printf("close_outline()\n");
}
#endif
