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

/***************************** I F A C E . C *********************************
 *                                                                           *
 * This module provides a layer to make Speedo function calls to and         *
 * from it compatible with Fontware 2.X function calls.                      *
 *                                                                           *
 ****************************************************************************/

#include "linux/libcwrap.h"
#include "speedo.h"						/* General definitions for Speedo */
#include <stdio.h>
#ifndef FONTMODULE
#include <math.h>
#else
#include "xf86_ansic.h"
#endif

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

#define  PI     3.1415926536			/* pi */
#define  PTPERINCH   72.2892			/* nbr points per inch, exactly! */

#define  BIT8           0x0100
#define  BIT9           0x0200
#define  BIT10          0x0400
#define  BIT11          0x0800
#define  BIT12          0x1000
#define  BIT13          0x2000
#define  BIT14          0x4000
#define  BIT15          0x8000

#define   READ      0

typedef short bool16;

typedef int bool;

typedef struct
{
	bool16 left;
	bool16 right;
	bool16 top;
	bool16 bottom;
} lrtb;


typedef struct
{
	buff_t *pfont;						/* Pointer to font data                    */
	ufix16 mode;						/* what mode is the font generator in      */
	double point_size_x;				/* Point size in X dimension               */
	double point_size_y;				/* Point size in Y dimension               */
	double res_hor;						/* Horizontal resolution of output device  */
	double res_ver;						/* Vertical resolution of output device    */
	double rot_angle;					/* Rotation angle in degrees (clockwise)   */
	double obl_angle;					/* Obliquing angle in degrees (clockwise)  */
	bool16 whitewrite;					/* if T, generate bitmaps for whitewriters */
	fix15 thresh;						/* Scan conversion threshold               *
										 * Thickens characters on each edge by     *
										 * <thresh> sub-pixels                     */
	bool16 import_widths;				/* Use imported width table                */
	lrtb clip;							/* Clip to standard character cell         */
	lrtb squeeze;						/* Squeeze to standard character cell      */
	bool16 bogus_mode;					/* if T, ignore plaid data                 */
} comp_char_desc;						/* character attributes for scan conv      */

void fw_reset(SPD_PROTO_DECL1);									/* Fontware 2.X reset call                 */
void fw_set_specs(SPD_PROTO_DECL2 comp_char_desc *pspecs);		/* Fontware 2.X set specs call             */
bool fw_make_char(SPD_PROTO_DECL2 ufix16 char_index);			/* Fontware 2.X make character call        */

static buff_t *pfont;

static fix15 set_width_x;

static specs_t specsarg;


void fw_reset(SPD_PROTO_DECL1)
{
	sp_reset(SPD_GARG);
}


static fix31 make_mult(double point_size, double resolution)
{
	return floor((point_size * resolution * 65536.0) / (double) PTPERINCH + 0.5);
}


/*  Fontware 2.X character generator call to set font specifications
 *  compc -- pointer to structure containing scan conversion parameters.
 *   ->compf -- compressed font data structure
 *   ->point_size_x -- x pointsize
 *   ->point_size_y -- y pointsize
 *   ->res_hor -- horizontal pixels per inch
 *   ->res_ver -- vertical pixels per inch
 *   ->rot_angle -- rotation angle in degrees (clockwise)
 *   ->obl_angle -- obliquing angle in degrees (clockwise)
 *   ->whitewrite -- if true, generate bitmaps for whitewriters
 *   ->thresh -- scan-conversion threshold
 *   ->import_widths -- if true, use external width table
 *   ->clip.left -- clips min x at left of emsquare
 *   ->clip.right -- clips max x at right of emsquare
 *   ->clip.bottom -- clips min x at bottom of emsquare
 *   ->clip.top -- clips max x at top of emsquare
 *   ->squeeze.left -- squeezes min x at left of emsquare
 *   ->squeeze.right, .top, .bottom  &c
 *   ->sw_fixed -- if TRUE, match pixel widths to scaled outline widths
 *   ->bogus_mode -- ignore plaid data if TRUE
 */
void fw_set_specs(SPD_PROTO_DECL2 comp_char_desc *pspecs)
{
	fix15 irot;
	fix15 iobl;
	fix15 x_trans_type;
	fix15 y_trans_type;
	fix31 xx_mult;
	fix31 xy_mult;
	fix31 yx_mult;
	fix31 yy_mult;
	double sinrot, cosrot, tanobl;
	double x_distortion;
	double pixperem_h;
	double pixperem_v;
	double point_size_x;
	double point_size_y;
	double res_hor;
	double res_ver;
	fix15 mode;

	irot = floor(pspecs->rot_angle + 0.5);
	iobl = floor(pspecs->obl_angle + 0.5);
	if (iobl > 85)
		iobl = 85;
	if (iobl < -85)
		iobl = -85;
	if ((irot % 90) == 0)
	{
		x_trans_type = y_trans_type = irot / 90 & 0x0003;
		if (iobl != 0)
		{
			if (x_trans_type & 0x01)
				y_trans_type = 4;
			else
				x_trans_type = 4;
		}
	} else if (((irot + iobl) % 90) == 0)
	{
		x_trans_type = y_trans_type = (irot + iobl) / 90 & 0x0003;
		if (iobl != 0)
		{
			if (x_trans_type & 0x01)
				x_trans_type = 4;
			else
				y_trans_type = 4;
		}
	} else
	{
		x_trans_type = y_trans_type = 4;
	}

	point_size_x = pspecs->point_size_x;
	point_size_y = pspecs->point_size_y;
	res_hor = pspecs->res_hor;
	res_ver = pspecs->res_ver;

	switch (x_trans_type)
	{
	case 0:
		xx_mult = make_mult(point_size_x, res_hor);
		xy_mult = 0;
		break;

	case 1:
		xx_mult = 0;
		xy_mult = make_mult(point_size_y, res_hor);
		break;

	case 2:
		xx_mult = -make_mult(point_size_x, res_hor);
		xy_mult = 0;
		break;

	case 3:
		xx_mult = 0;
		xy_mult = -make_mult(point_size_y, res_hor);
		break;

	default:
		sinrot = sin((double) irot * PI / 180.);
		cosrot = cos((double) irot * PI / 180.);
		tanobl = tan((double) iobl * PI / 180.);
		x_distortion = point_size_x / point_size_y;
		pixperem_h = point_size_y * res_hor / (double) PTPERINCH;	/* this is NOT a bug */
		xx_mult = floor(cosrot * x_distortion * pixperem_h * 65536.0 + 0.5);
		xy_mult = floor((sinrot + cosrot * tanobl) * pixperem_h * 65536.0 + 0.5);
		break;
	}

	switch (y_trans_type)
	{
	case 0:
		yx_mult = 0;
		yy_mult = make_mult(point_size_y, res_ver);
		break;

	case 1:
		yx_mult = -make_mult(point_size_x, res_hor);
		yy_mult = 0;
		break;

	case 2:
		yx_mult = 0;
		yy_mult = -make_mult(point_size_y, res_ver);
		break;

	case 3:
		yx_mult = make_mult(point_size_x, res_ver);
		yy_mult = 0;
		break;

	default:
		sinrot = sin((double) irot * PI / 180.);
		cosrot = cos((double) irot * PI / 180.);
		tanobl = tan((double) iobl * PI / 180.);
		x_distortion = point_size_x / point_size_y;
		pixperem_v = point_size_y * res_ver / (double) PTPERINCH;
		yx_mult = floor(-sinrot * x_distortion * pixperem_v * 65536.0 + 0.5);
		yy_mult = floor((cosrot - sinrot * tanobl) * pixperem_v * 65536.0 + 0.5);
		break;
	}

	specsarg.xxmult = xx_mult;
	specsarg.xymult = xy_mult;
	specsarg.xoffset = 0;
	specsarg.yxmult = yx_mult;
	specsarg.yymult = yy_mult;
	specsarg.yoffset = 0;
	specsarg.out_info = 0;

	/* Select processing mode */
	switch (pspecs->mode)
	{
	case 1:
		if (pspecs->whitewrite)			/* White-write requested? */
		{
			mode = 1;
		} else
		{
			mode = 0;
		}
		break;

	case 2:
		mode = 2;
		break;

	default:
		mode = pspecs->mode;
		break;
	}

	if (pspecs->bogus_mode)				/* Linear transformation requested? */
	{
		mode |= BIT4;					/* Set linear tranformation flag */
	}

	if (pspecs->import_widths)			/* Imported widths requested? */
	{
		mode |= BIT6;					/* Set imported width flag */
	}

	if (pspecs->clip.left)				/* Clip left requested? */
	{
		mode |= BIT8;					/* Set clip left flag */
	}

	if (pspecs->clip.right)				/* Clip right requested? */
	{
		mode |= BIT9;					/* Set clip right flag */
	}

	if (pspecs->clip.top)				/* Clip top requested? */
	{
		mode |= BIT10;					/* Set clip top flag */
	}

	if (pspecs->clip.bottom)			/* Clip bottom requested? */
	{
		mode |= BIT11;					/* Set clip bottom flag */
	}

	if (pspecs->squeeze.left)			/* Squeeze left requested? */
	{
		mode |= BIT12;					/* Set squeeze left flag */
	}

	if (pspecs->squeeze.right)			/* Squeeze right requested? */
	{
		mode |= BIT13;					/* Set squeeze right flag */
	}

	if (pspecs->squeeze.top)			/* Squeeze top requested? */
	{
		mode |= BIT14;					/* Set squeeze top flag */
	}

	if (pspecs->squeeze.bottom)			/* Squeeze bottom requested? */
	{
		mode |= BIT15;					/* Set squeeze bottom flag */
	}

	specsarg.flags = mode;

	sp_set_specs(SPD_GARGS &specsarg, pspecs->pfont);
}

bool fw_make_char(SPD_PROTO_DECL2 ufix16 char_index)
{
	return sp_make_char(SPD_GARGS char_index);
}


#if INCL_LCD
/*
 * Called by Speedo character generator to request that character
 * data be loaded from the font file.
 * This is a dummy function that assumes that the entire font has
 * been loaded.
 */
boolean sp_load_char_data(SPD_PROTO_DECL2 long file_offset, fix15 no_bytes, fix15 cb_offset, buff_t *char_data)
{
	SPD_GUNUSED
#if DEBUG
	printf("fw_load_char_data(%ld, %d, %d)\n", file_offset, no_bytes, char_offset);
#endif
	char_data->org = pfont->org + file_offset;
	char_data->no_bytes = no_bytes;
	return TRUE;
}
#endif


/* 
 * Called by Speedo character generator to initialize a buffer prior
 * to receiving bitmap data.
 */
void sp_open_bitmap(SPD_PROTO_DECL2
	fix31 xorg,								/* X origin */
	fix31 yorg,								/* Y origin */
	fix15 xsize,							/* width of bitmap */
	fix15 ysize)							/* height of bitmap */
{
	fix15 xmin, xmax, ymin, ymax;

#if DEBUG
	printf("sp_open_bitmap:\n");
	printf("    Bounding box is (%d, %d, %d, %d)\n", xmin, ymin, xmax, ymax);
#endif

	xmin = xorg >> 16;
	ymin = yorg >> 16;
	xmax = xmin + xsize;
	ymax = ymin + ysize;

	set_width_x = ((sp_globals.set_width.x >> 15) + 1) >> 1;
	(*sp_globals.bitmap_device.p_open_bitmap)(SPD_GARGS xmin, xmax, ymin, ymax);
}

/* 
 * Called by Speedo character generator to write one row of pixels 
 * into the generated bitmap character.                               
 */
void sp_set_bitmap_bits(SPD_PROTO_DECL2 fix15 y, fix15 x1, fix15 x2)
{
#if DEBUG
	printf("set_bitmap_bits(%d, %d, %d)\n", y, x1, x2);
#endif

	(*sp_globals.bitmap_device.p_set_bits)(SPD_GARGS y, x1, x2);
}

/* 
 * Called by Speedo character generator to indicate all bitmap data
 * has been generated.
 */
void sp_close_bitmap(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("close_bitmap()\n");
#endif

	(*sp_globals.bitmap_device.p_close_bitmap)(SPD_GARG);
}

/*
 * Called by Speedo character generator to initialize prior to
 * outputting scaled outline data.
 */
void sp_open_outline(SPD_PROTO_DECL2
	fix31 sw_x,								/* X component of escapement vector */
	fix31 sw_y,								/* Y component of escapement vector */
	fix31 xmin,								/* Minimum X value in outline */
	fix31 xmax,								/* Maximum X value in outline */
	fix31 ymin,								/* Minimum Y value in outline */
	fix31 ymax)								/* Maximum Y value in outline */
{
#if DEBUG
	printf("open_outline(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) sw_x / 65536.0, (double) sw_y / 65536.0,
		   (double) xmin / 65536.0, (double) xmax / 65536.0,
		   (double) ymin / 65536.0, (double) ymax / 65536.0);
#endif

	set_width_x = ((sw_x >> 15) + 1) >> 1;
	(*sp_globals.outline_device.p_open_outline)(SPD_GARGS set_width_x, set_width_x, xmin, xmax, ymin, ymax);
}

/*
 * Called by Speedo character generator to initialize prior to
 * outputting scaled outline data for a sub-character in a compound
 * character.
 */
void sp_start_sub_char(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("start_sub_char()\n");
#endif

	(*sp_globals.outline_device.p_start_sub_char)(SPD_GARG);
}

void sp_end_sub_char(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("end_sub_char()\n");
#endif

	(*sp_globals.outline_device.p_end_sub_char)(SPD_GARG);
}

/*
 * Called by Speedo character generator at the start of each contour
 * in the outline data of the character.
 */
void sp_start_contour(SPD_PROTO_DECL2
	fix31 x,								/* X coordinate of start point in 1/65536 pixels */
	fix31 y,								/* Y coordinate of start point in 1/65536 pixels */
	boolean outside)						/* TRUE if curve encloses ink (Counter-clockwise) */
{
	double realx, realy;

	realx = (double) x / 65536.0;
	realy = (double) y / 65536.0;

#if DEBUG
	printf("start_curve(%3.1f, %3.1f, %s)\n", realx, realy, outside ? "outside" : "inside");
#endif

	(*sp_globals.outline_device.p_start_contour)(SPD_GARGS realx, realy, outside);
}

/*
 * Called by Speedo character generator once for each curve in the
 * scaled outline data of the character. This is only called if curve
 * output is enabled in the sp_set_specs() call.
 */
void sp_curve_to(SPD_PROTO_DECL2
	fix31 x1,								/* X coordinate of first control point in 1/65536 pixels */
	fix31 y1,								/* Y coordinate of first control  point in 1/65536 pixels */
	fix31 x2,								/* X coordinate of second control point in 1/65536 pixels */
	fix31 y2,								/* Y coordinate of second control point in 1/65536 pixels */
	fix31 x3,								/* X coordinate of curve end point in 1/65536 pixels */
	fix31 y3)								/* Y coordinate of curve end point in 1/65536 pixels */
{
#if DEBUG
	printf("curve_to(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x1 / 65536.0, (double) y1 / 65536.0,
		   (double) x2 / 65536.0, (double) y2 / 65536.0,
		   (double) x3 / 65536.0, (double) y3 / 65536.0);
#endif
	(*sp_globals.outline_device.p_curve)(SPD_GARG x1, y1, x2, y2, x3, y3);
}


/*
 * Called by Speedo character generator onece for each vector in the
 * scaled outline data for the character. This include curve data that has
 * been sub-divided into vectors if curve output has not been enabled
 * in the sp_set_specs() call.
 */
void sp_line_to(SPD_PROTO_DECL2
	fix31 x,								/* X coordinate of vector end point in 1/65536 pixels */
	fix31 y)								/* Y coordinate of vector end point in 1/65536 pixels */
{
	double realx, realy;

	realx = (double) x / 65536.0;
	realy = (double) y / 65536.0;

#if DEBUG
	printf("line_to(%3.1f, %3.1f)\n", realx, realy);
#endif

	(*sp_globals.outline_device.p_line)(SPD_GARGS realx, realy);
}

/*
 * Called by Speedo character generator at the end of each contour
 * in the outline data of the character.
 */
void sp_close_contour(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("close_curve()\n");
#endif

	(*sp_globals.outline_device.p_close_contour)(SPD_GARG);
}

/*
 * Called by Speedo character generator at the end of output of the
 * scaled outline of the character.
 */
void sp_close_outline(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("close_outline()\n");
#endif

	(*sp_globals.outline_device.p_close_outline)(SPD_GARG);
}
