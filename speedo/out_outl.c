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


/**************************** O U T _ 2 _ 1 . C ******************************
 *                                                                           *
 * This is the standard output module for vector output mode.                *
 *                                                                           *
 ****************************************************************************/

#include "linux/libcwrap.h"
#include <stdio.h>
#include "spdo_prv.h"					/* General definitions for Speedo     */


#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif

/* the following macro is used to limit points on the outline to the bounding box */

#define RANGECHECK(value,min,max) (((value) >= (min) ? (value) : (min)) < (max) ? (value) : (max))


#if INCL_OUTLINE /* whole file */



/*
 * init_out2() is called by sp_set_specs() to initialize the output module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
boolean sp_init_outline(SPD_PROTO_DECL2 specs_t *specsarg)
{
	SPD_GUNUSED
#if DEBUG
	printf("INIT_OUT_2()\n");
#endif
	if (specsarg->flags & (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM))
		return FALSE;					/* Clipping not supported */
	return TRUE;
}


/*
 * If two or more output modules are included in the configuration, begin_char2()
 * is called by begin_char() to signal the start of character output data.
 * If only one output module is included in the configuration, begin_char() is 
 * called by sp_make_simp_char() and sp_make_comp_char().
 */
boolean sp_begin_char_outline(SPD_PROTO_DECL2 
	fix31 x, fix31 y,	/* End of escapement vector (sub-pixels) */
	fix31 minx, fix31 miny,	/* Bottom left corner of bounding box */
	fix31 maxx, fix31 maxy)							/* Top right corner of bounding box */
{
	fix31 set_width_x;
	fix31 set_width_y;
	fix31 xmin;
	fix31 xmax;
	fix31 ymin;
	fix31 ymax;

#if DEBUG
	printf("BEGIN_CHAR_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n",
		   (double) x / (double) sp_globals.onepix, (double) y / (double) sp_globals.onepix,
		   (double) minx / (double) sp_globals.onepix, (double) miny / (double) sp_globals.onepix,
		   (double) maxx / (double) sp_globals.onepix, (double) maxy / (double) sp_globals.onepix);
#endif
	sp_globals.poshift = 16 - sp_globals.pixshift;
	set_width_x = (fix31) x << sp_globals.poshift;
	set_width_y = (fix31) y << sp_globals.poshift;
	xmin = minx << sp_globals.poshift;
	xmax = maxx << sp_globals.poshift;
	ymin = miny << sp_globals.poshift;
	ymax = maxy << sp_globals.poshift;
	sp_globals.xmin = minx;
	sp_globals.xmax = maxx;
	sp_globals.ymin = miny;
	sp_globals.ymax = maxy;
	open_outline(set_width_x, set_width_y, xmin, xmax, ymin, ymax);
	return TRUE;
}


/*
 * If two or more output modules are included in the configuration, begin_sub_char2()
 * is called by begin_sub_char() to signal the start of sub-character output data.
 * If only one output module is included in the configuration, begin_sub_char() is 
 * called by sp_make_comp_char().
 */
void sp_begin_sub_char_outline(SPD_PROTO_DECL2 
	fix31 x, fix31 y,	/* End of sub-char escapement vector */
	fix31 minx, fix31 miny,							/* Bottom left corner of sub-char bounding box */
	fix31 maxx, fix31 maxy)							/* Top right corner of sub-char bounding box */
{
	SPD_GUNUSED
#if DEBUG
	printf("BEGIN_SUB_CHAR_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x / (double) sp_globals.onepix, (double) y / (double) sp_globals.onepix,
		   (double) minx / (double) sp_globals.onepix, (double) miny / (double) sp_globals.onepix,
		   (double) maxx / (double) sp_globals.onepix, (double) maxy / (double) sp_globals.onepix);
#endif
	UNUSED(x);
	UNUSED(y);
	UNUSED(minx);
	UNUSED(miny);
	UNUSED(maxx);
	UNUSED(maxy);
}


/*
 * If two or more output modules are included in the configuration, begin_contour2()
 * is called by begin_contour() to define the start point of a new contour
 * and to indicate whether it is an outside (counter-clockwise) contour
 * or an inside (clockwise) contour.
 * If only one output module is included in the configuration, begin_sub_char() is 
 * called by sp_proc_outl_data().
 */
void sp_begin_contour_outline(SPD_PROTO_DECL2 
	fix31 x1, fix31 y1,	/* Start point of contour */
	boolean outside)						/* TRUE if outside (counter-clockwise) contour */
{
	fix15 x, y;

#if DEBUG
	printf("BEGIN_CONTOUR_2(%3.1f, %3.1f, %s)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix, outside ? "outside" : "inside");
#endif
	x = RANGECHECK(x1, sp_globals.xmin, sp_globals.xmax);
	y = RANGECHECK(y1, sp_globals.ymin, sp_globals.ymax);

	start_contour((fix31) x << sp_globals.poshift, (fix31) y << sp_globals.poshift, outside);
}


/*
 * If two or more output modules are included in the configuration, curve2()
 * is called by curve() to output one curve segment.
 * If only one output module is included in the configuration, curve() is 
 * called by sp_proc_outl_data().
 * This function is only called when curve output is enabled.
 */
void sp_curve_outline(SPD_PROTO_DECL2 
	fix31 x1, fix31 y1,	/* First control point of Bezier curve */
	fix31 x2, fix31 y2,								/* Second control point of Bezier curve */
	fix31 x3, fix31 y3,								/* End point of Bezier curve */
	fix15 depth)
{
#if DEBUG
	printf("CURVE_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix,
		   (double) x2 / (double) sp_globals.onepix, (double) y2 / (double) sp_globals.onepix,
		   (double) x3 / (double) sp_globals.onepix, (double) y3 / (double) sp_globals.onepix);
#endif
	UNUSED(depth);
	x1 = RANGECHECK(x1, sp_globals.xmin, sp_globals.xmax);
	y1 = RANGECHECK(y1, sp_globals.ymin, sp_globals.ymax);

	x2 = RANGECHECK(x2, sp_globals.xmin, sp_globals.xmax);
	y2 = RANGECHECK(y2, sp_globals.ymin, sp_globals.ymax);

	x3 = RANGECHECK(x3, sp_globals.xmin, sp_globals.xmax);
	y3 = RANGECHECK(y3, sp_globals.ymin, sp_globals.ymax);

	curve_to(x1 << sp_globals.poshift, y1 << sp_globals.poshift,
			 x2 << sp_globals.poshift, y2 << sp_globals.poshift,
			 x3 << sp_globals.poshift, y3 << sp_globals.poshift);
}


/*
 * If two or more output modules are included in the configuration, line2()
 * is called by line() to output one vector.
 * If only one output module is included in the configuration, line() is 
 * called by sp_proc_outl_data(). If curve output is enabled, line() is also
 * called by sp_split_curve().
 */
void sp_line_outline(SPD_PROTO_DECL2 fix31 x1, fix31 y1)	/* End point of vector */
{
#if DEBUG
	printf("LINE_2(%3.1f, %3.1f)\n", (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix);
#endif
	x1 = RANGECHECK(x1, sp_globals.xmin, sp_globals.xmax);
	y1 = RANGECHECK(y1, sp_globals.ymin, sp_globals.ymax);

	line_to(x1 << sp_globals.poshift, y1 << sp_globals.poshift);
}


/*
 * If two or more output modules are included in the configuration, end_contour2()
 * is called by end_contour() to signal the end of a contour.
 * If only one output module is included in the configuration, end_contour() is 
 * called by sp_proc_outl_data().
 */
void sp_end_contour_outline(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("END_CONTOUR_2()\n");
#endif
	close_contour();
}


/*
 * If two or more output modules are included in the configuration, end_sub_char2()
 * is called by end_sub_char() to signal the end of sub-character data.
 * If only one output module is included in the configuration, end_sub_char() is 
 * called by sp_make_comp_char().
 */
void sp_end_sub_char_outline(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("END_SUB_CHAR_2()\n");
#endif
	end_sub_char();
}


/*
 * If two or more output modules are included in the configuration, end_char2()
 * is called by end_char() to signal the end of the character data.
 * If only one output module is included in the configuration, end_char() is 
 * called by sp_make_simp_char() and sp_make_comp_char().
 * Returns TRUE if output process is complete
 * Returns FALSE to repeat output of the transformed data beginning
 * with the first contour (of the first sub-char if compound).
 */
boolean sp_end_char_outline(SPD_PROTO_DECL1)
{
#if DEBUG
	printf("END_CHAR_2()\n");
#endif
	close_outline();
	return TRUE;
}

#else /* INCL_OUTLINE */

extern int _I_dont_care_that_ISO_C_forbids_an_empty_source_file_;

#endif
