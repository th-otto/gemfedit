/*

Copyright 1989-1991, Bitstream Inc., Cambridge, MA.
You are hereby granted permission under all Bitstream propriety rights to
use, copy, modify, sublicense, sell, and redistribute the Bitstream Speedo
software and the Bitstream Charter outline font for any purpose and without
restrictions; provided, that this notice is left intact on all copies of such
software or font and that Bitstream's trademark is acknowledged as shown below
on all unmodified copies of such font.

BITSTREAM CHARTER is a registered trademark of Bitstream Inc.


BITSTREAM INC. DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
WITHOUT LIMITATION THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.  BITSTREAM SHALL NOT BE LIABLE FOR ANY DIRECT OR INDIRECT
DAMAGES, INCLUDING BUT NOT LIMITED TO LOST PROFITS, LOST DATA, OR ANY OTHER
INCIDENTAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF OR IN ANY WAY CONNECTED
WITH THE SPEEDO SOFTWARE OR THE BITSTREAM CHARTER OUTLINE FONT.

*/


/*************************** N S A M P L E . C *******************************
 *                                                                           *
 *                 SPEEDO CHARACTER GENERATOR TEST MODULE                    *
 *                                                                           *
 * This is an illustration of what external resources are required to        *
 * load a Speedo outline and use the Speedo character generator to generate  *
 * bitmaps or scaled outlines according to the desired specification.        *                                                    *
 *                                                                           *
 * This program loads a Speedo outline, defines a set of character           *
 * generation specifications, generates bitmap (or outline) data for each    *
 * character in the font and prints them on the standard output.             *
 *                                                                           *
 * If the font buffer is too small to hold the entire font, the first        *
 * part of the font is loaded. Character data is then loaded dynamically     *
 * as required.                                                              *
 *                                                                           *
 ****************************************************************************/

#include "linux/libcwrap.h"
#include <stdio.h>
#include <stddef.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>

int main(int argc, char *argv[]);

#include "speedo.h"						/* General definition for make_bmap */

#define DEBUG  0

#if DEBUG
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#define MAX_BITS  256					/* Max line length of generated bitmap */

static char pathname[100];				/* Name of font file to be output */

static ufix8 *font_buffer;		/* Pointer to allocated Font buffer */

static ufix8 *char_buffer;		/* Pointer to allocate Character buffer */

static buff_t font;						/* Buffer descriptor for font data */

static FILE *fdescr;					/* Speedo outline file descriptor */

static ufix16 char_index;				/* Index of character to be generated */

static ufix16 char_id;					/* Character ID */

static ufix16 minchrsz;					/* minimum character buffer size */

static fix15 raswid;					/* raster width  */

static fix15 rashgt;					/* raster height */

static fix15 offhor;					/* horizontal offset from left edge of emsquare */

static fix15 offver;					/* vertical offset from baseline */

static fix15 y_cur;						/* Current y value being generated and printed */

static char line_of_bits[2 * MAX_BITS + 1];	/* Buffer for row of generated bits */

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_SCREEN || INCL_2D
static bitmap_t bfuncs = { sp_open_bitmap, sp_set_bitmap_bits, sp_close_bitmap };
#endif
#if INCL_OUTLINE
static outline_t ofuncs = {
	sp_open_outline,
	sp_start_sub_char,
	sp_end_sub_char,
	sp_start_contour,
	sp_curve_to,
	sp_line_to,
	sp_close_contour,
	sp_close_outline
};
#endif
#endif


/*
 * Reads 2-byte field from font buffer 
 */
static fix15 read_2b(ufix8 *pointer)
{
	fix15 temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer);
	return temp;
}


/*
 * Reads 4-byte field from font buffer 
 */
static fix31 read_4b(ufix8 *pointer)
{
	fix31 temp;

	temp = *pointer++;
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer++);
	temp = (temp << 8) + *(pointer);
	return temp;
}


int main(int argc, char **argv)
{
	ufix16 bytes_read;					/* Number of bytes read from font file */
	specs_t specs;						/* Bundle of character generation specs  */
	int first_char_index;				/* Index of first character in font */
	int no_layout_chars;				/* number of characters in layout */
	ufix32 i;
	ufix32 minbufsz;					/* minimum font buffer size to allocate */
	const ufix8 *key;
	ufix8 temp[FH_FBFSZ + 4];							/* temp buffer for first 16 bytes of font */
	
	if (argc != 2)
	{
		fprintf(stderr, "Usage: nsample {fontfile}\n\n");
		exit(1);
	}

	sprintf(pathname, argv[1]);

	/* Load Speedo outline file */
	fdescr = fopen(pathname, "rb");
	if (fdescr == NULL)
	{
		printf("****** Cannot open file %s\n", pathname);
		return 1;
	}

	/* get minimum font buffer size - read first 16 bytes to get the minimum
   size field from the header, then allocate buffer dynamically  */

	bytes_read = fread(temp, sizeof(ufix8), sizeof(temp), fdescr);

	if (bytes_read != sizeof(temp))
	{
		printf("****** Error on reading %s: %x\n", pathname, bytes_read);
		fclose(fdescr);
		return 1;
	}
#if INCL_LCD
	minbufsz = (ufix32) read_4b(temp + FH_FBFSZ);
#else
	minbufsz = (ufix32) read_4b(temp + FH_FNTSZ);
	if (minbufsz >= 0x10000)
	{
		printf("****** Cannot process fonts greater than 64K - use dynamic character loading configuration option\n");
		fclose(fdescr);
		return 1;
	}
#endif

	font_buffer = (ufix8 *) malloc(minbufsz);

	if (font_buffer == NULL)
	{
		printf("****** Unable to allocate memory for font buffer\n");
		fclose(fdescr);
		return 1;
	}
#if DEBUG
	printf("Loading font file %s\n", pathname);
#endif

	fseek(fdescr, 0, SEEK_SET);
	bytes_read = fread((ufix8 *) font_buffer, sizeof(ufix8), minbufsz, fdescr);
	if (bytes_read == 0)
	{
		printf("****** Error on reading %s: %x\n", pathname, bytes_read);
		fclose(fdescr);
		return 1;
	}

#if INCL_LCD
	/* now allocate minimum character buffer */

	minchrsz = read_2b(font_buffer + FH_CBFSZ);
	char_buffer = (ufix8 *) malloc(minchrsz);

	if (char_buffer == NULL)
	{
		printf("****** Unable to allocate memory for character buffer\n");
		fclose(fdescr);
		return 1;
	}
#endif

	/* Initialization */
	sp_reset();							/* Reset Speedo character generator */

	font.org = font_buffer;
	font.no_bytes = bytes_read;

	key = sp_get_key(&font);
	if (key == NULL)
	{
		printf("Unable to use fonts for customer number %d\n", sp_get_cust_no(&font));
		fclose(fdescr);
		return 1;
	} else
	{
		sp_set_key(key);					/* Set decryption key */
	}

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_SCREEN || INCL_2D
	sp_set_bitmap_device(&bfuncs, sizeof(bfuncs));
#endif
#if INCL_OUTLINE
	sp_set_outline_device(&ofuncs, sizeof(ofuncs));
#endif
#endif

	first_char_index = read_2b(font_buffer + FH_FCHRF);
	no_layout_chars = read_2b(font_buffer + FH_NCHRL);

	/* Set specifications for character to be generated */
	specs.xxmult = 25L << 16;			/* Coeff of X to calculate X pixels */
	specs.xymult = 0L << 16;			/* Coeff of Y to calculate X pixels */
	specs.xoffset = 0L << 16;			/* Position of X origin */
	specs.yxmult = 0L << 16;			/* Coeff of X to calculate Y pixels */
	specs.yymult = 25L << 16;			/* Coeff of Y to calculate Y pixels */
	specs.yoffset = 0L << 16;			/* Position of Y origin */
	specs.output_mode = MODE_BLACK;
	specs.flags = 0;					/* Mode flags */
	specs.out_info = NULL;


	/* Set character generation specifications */
	if (!sp_set_specs(&specs, &font))
	{
		printf("****** Cannot set requested specs\n");
	} else
	{
		for (i = 0; i < no_layout_chars; i++)	/* For each character in font */
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
			{
				if (!sp_make_char(char_index))
				{
					printf("****** Cannot generate character %d\n", char_index);
				}
			}
		}
	}

	fclose(fdescr);
	
	return 0;
}


/*
 * Called by Speedo character generator to request that character
 * data be loaded from the font file into a character data buffer.
 * The character buffer offset is zero for all characters except elements
 * of compound characters. If a single buffer is allocated for character
 * data, cb_offset ensures that sub-character data is loaded after the
 * top-level compound character.
 * Returns a pointer to a buffer descriptor.
 */
#if INCL_LCD
boolean sp_load_char_data(
	long file_offset,						/* Offset in bytes from the start of the font file */
	fix15 no_bytes,							/* Number of bytes to be loaded */
	fix15 cb_offset,						/* Offset in bytes from start of char buffer */
	buff_t *char_data)
{
	int bytes_read;

#if DEBUG
	printf("\nCharacter data(%ld, %d, %d) requested\n", file_offset, no_bytes, cb_offset);
#endif
	if (fseek(fdescr, file_offset, SEEK_SET) != 0)
	{
		fprintf(stderr, "****** Error in seeking character\n");
		return FALSE;
	}

	if ((no_bytes + cb_offset) > minchrsz)
	{
		fprintf(stderr, "****** Character buffer overflow\n");
		return FALSE;
	}

	bytes_read = fread((char_buffer + cb_offset), sizeof(ufix8), no_bytes, fdescr);
	if (bytes_read != no_bytes)
	{
		fprintf(stderr, "****** Error on reading character data\n");
		return FALSE;
	}
#if DEBUG
	printf("Character data loaded\n");
#endif

	char_data->org = char_buffer + cb_offset;
	char_data->no_bytes = no_bytes;
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
}


/* 
 * Called by Speedo character generator to initialize a buffer prior
 * to receiving bitmap data.
 */
void sp_open_bitmap(
	fix31 xorg,								/* Pixel boundary at left extent of bitmap character */
	fix31 yorg,								/* Pixel boundary at right extent of bitmap character */
	fix15 xsize,							/* Pixel boundary of bottom extent of bitmap character */
	fix15 ysize)							/* Pixel boundary of top extent of bitmap character */
{
	fix15 i;

#if DEBUG
	printf("open_bitmap(%3.1f, %3.1f, %d, %d)\n",
		   (double) xorg / 65536.0, (double) yorg / 65536.0, (int) xsize, (int) ysize);
#endif
	raswid = xsize;
	rashgt = ysize;
	offhor = (fix15) (xorg >> 16);
	offver = (fix15) (yorg >> 16);

	if (raswid > MAX_BITS)
		raswid = MAX_BITS;

	printf("\nCharacter index = %d, ID = %d\n", char_index, char_id);
	printf("X offset  = %d\n", offhor);
	printf("Y offset  = %d\n\n", offver);
	for (i = 0; i < raswid; i++)
	{
		line_of_bits[i << 1] = '.';
		line_of_bits[(i << 1) + 1] = ' ';
	}
	line_of_bits[raswid << 1] = '\0';
	y_cur = 0;
}


/* 
 * Called by Speedo character generator to write one row of pixels 
 * into the generated bitmap character.                               
 */
void sp_set_bitmap_bits(
	fix15 y,								/* Scan line (0 = first row above baseline) */
	fix15 xbit1,							/* Pixel boundary where run starts */
	fix15 xbit2)							/* Pixel boundary where run ends */
{
	fix15 i;

#if DEBUG
	printf("set_bitmap_bits(%d, %d, %d)\n", (int) y, (int) xbit1, (int) xbit2);
#endif
	/* Clip runs beyond end of buffer */
	if (xbit1 > MAX_BITS)
		xbit1 = MAX_BITS;

	if (xbit2 > MAX_BITS)
		xbit2 = MAX_BITS;

	/* Output backlog lines if any */
	while (y_cur != y)
	{
		printf("    %s\n", line_of_bits);
		for (i = 0; i < raswid; i++)
		{
			line_of_bits[i << 1] = '.';
		}
		y_cur++;
	}

	/* Add bits to current line */
	for (i = xbit1; i < xbit2; i++)
	{
		line_of_bits[i << 1] = 'X';
	}
}

/* 
 * Called by Speedo character generator to indicate all bitmap data
 * has been generated.
 */
void sp_close_bitmap(void)
{
#if DEBUG
	printf("close_bitmap()\n");
#endif
	printf("    %s\n", line_of_bits);
}


/*
 * Called by Speedo character generator to initialize prior to
 * outputting scaled outline data.
 */
#if INCL_OUTLINE
void sp_open_outline(
	fix31 x_set_width,						/* Transformed escapement vector */
	fix31 y_set_width,
	fix31 xmin,								/* Minimum X value in outline */
	fix31 xmax,								/* Maximum X value in outline */
	fix31 ymin,								/* Minimum Y value in outline */
	fix31 ymax)								/* Maximum Y value in outline */
{
	printf("\nopen_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x_set_width / 65536.0, (double) y_set_width / 65536.0,
		   (double) xmin / 65536.0, (double) xmax / 65536.0,
		   (double) ymin / 65536.0, (double) ymax / 65536.0);
}


/*
 * Called by Speedo character generator to initialize prior to
 * outputting scaled outline data for a sub-character in a compound
 * character.
 */
void sp_start_sub_char(void)
{
	printf("start_sub_char()\n");
}

void sp_end_sub_char(void)
{
	printf("end_sub_char()\n");
}


/*
 * Called by Speedo character generator at the start of each contour
 * in the outline data of the character.
 */
void sp_start_contour(
	fix31 x,								/* X coordinate of start point in 1/65536 pixels */
	fix31 y,								/* Y coordinate of start point in 1/65536 pixels */
	boolean outside)						/* TRUE if curve encloses ink (Counter-clockwise) */
{
	printf("start_contour(%3.1f, %3.1f, %s)\n", (double) x / 65536.0, (double) y / 65536.0, outside ? "outside" : "inside");
}

/*
 * Called by Speedo character generator onece for each curve in the
 * scaled outline data of the character. This function is only called if curve
 * output is enabled in the sp_set_specs() call.
 */
void sp_curve_to(
	fix31 x1,								/* X coordinate of first control point in 1/65536 pixels */
	fix31 y1,								/* Y coordinate of first control  point in 1/65536 pixels */
	fix31 x2,								/* X coordinate of second control point in 1/65536 pixels */
	fix31 y2,								/* Y coordinate of second control point in 1/65536 pixels */
	fix31 x3,								/* X coordinate of curve end point in 1/65536 pixels */
	fix31 y3)								/* Y coordinate of curve end point in 1/65536 pixels */
{
	printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x1 / 65536.0, (double) y1 / 65536.0,
		   (double) x2 / 65536.0, (double) y2 / 65536.0,
		   (double) x3 / 65536.0, (double) y3 / 65536.0);
}


/*
 * Called by Speedo character generator once for each vector in the
 * scaled outline data for the character. This include curve data that has
 * been sub-divided into vectors if curve output has not been enabled
 * in the sp_set_specs() call.
 */
void sp_line_to(fix31 x1, fix31 y1)
{
	printf("line_to(%3.1f, %3.1f)\n", (double) x1 / 65536.0, (double) y1 / 65536.0);
}


/*
 * Called by Speedo character generator at the end of each contour
 * in the outline data of the character.
 */
void sp_close_contour(void)
{
	printf("close_contour()\n");
}

/*
 * Called by Speedo character generator at the end of output of the
 * scaled outline of the character.
 */
void sp_close_outline(void)
{
	printf("close_outline()\n");
}

#endif
