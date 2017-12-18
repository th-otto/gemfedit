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
/***** GLOBAL VARIABLES *****/

/***** GLOBAL FUNCTIONS *****/

/***** EXTERNAL VARIABLES *****/

/***** EXTERNAL FUNCTIONS *****/

/***** STATIC VARIABLES *****/

/***** STATIC FUNCTIONS *****/


#if INCL_OUTLINE
/*
 * init_out2() is called by sp_set_specs() to initialize the output module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
boolean sp_init_outline(specs_t *specsarg)
{
#if DEBUG
	printf("INIT_OUT_2()\n");
#endif
	if (specsarg->flags & (CLIP_LEFT + CLIP_RIGHT + CLIP_TOP + CLIP_BOTTOM))
		return FALSE;					/* Clipping not supported */
	return (TRUE);
}
#endif

#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, begin_char2()
 * is called by begin_char() to signal the start of character output data.
 * If only one output module is included in the configuration, begin_char() is 
 * called by sp_make_simp_char() and sp_make_comp_char().
 */
boolean sp_begin_char_outline(
	point_t Psw,	/* End of escapement vector (sub-pixels) */
	point_t Pmin,	/* Bottom left corner of bounding box */
	Pmax)							/* Top right corner of bounding box */
{
	fix31 set_width_x;
	fix31 set_width_y;
	fix31 xmin;
	fix31 xmax;
	fix31 ymin;
	fix31 ymax;

#if DEBUG
	printf("BEGIN_CHAR_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n",
		   (real) Psw.x / (real) onepix, (real) Psw.y / (real) onepix,
		   (real) Pmin.x / (real) onepix, (real) Pmin.y / (real) onepix,
		   (real) Pmax.x / (real) onepix, (real) Pmax.y / (real) onepix);
#endif
	sp_globals.poshift = 16 - sp_globals.pixshift;
	set_width_x = (fix31) Psw.x << sp_globals.poshift;
	set_width_y = (fix31) Psw.y << sp_globals.poshift;
	xmin = (fix31) Pmin.x << sp_globals.poshift;
	xmax = (fix31) Pmax.x << sp_globals.poshift;
	ymin = (fix31) Pmin.y << sp_globals.poshift;
	ymax = (fix31) Pmax.y << sp_globals.poshift;
	sp_globals.xmin = Pmin.x;
	sp_globals.xmax = Pmax.x;
	sp_globals.ymin = Pmin.y;
	sp_globals.ymax = Pmax.y;
	open_outline(set_width_x, set_width_y, xmin, xmax, ymin, ymax);
	return TRUE;
}
#endif


#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, begin_sub_char2()
 * is called by begin_sub_char() to signal the start of sub-character output data.
 * If only one output module is included in the configuration, begin_sub_char() is 
 * called by sp_make_comp_char().
 */
void sp_begin_sub_char_outline(
	point_t Psw,	/* End of sub-char escapement vector */
	point_t Pmin,							/* Bottom left corner of sub-char bounding box */
	point_t Pmax)							/* Top right corner of sub-char bounding box */
{
#if DEBUG
	printf("BEGIN_SUB_CHAR_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n",
		   (real) Psw.x / (real) onepix, (real) Psw.y / (real) onepix,
		   (real) Pmin.x / (real) onepix, (real) Pmin.y / (real) onepix,
		   (real) Pmax.x / (real) onepix, (real) Pmax.y / (real) onepix);
#endif
	start_new_char();
}
#endif


/*
 * If two or more output modules are included in the configuration, begin_contour2()
 * is called by begin_contour() to define the start point of a new contour
 * and to indicate whether it is an outside (counter-clockwise) contour
 * or an inside (clockwise) contour.
 * If only one output module is included in the configuration, begin_sub_char() is 
 * called by sp_proc_outl_data().
 */
#if INCL_OUTLINE
void sp_begin_contour_outline(
	point_t P1,	/* Start point of contour */
	boolean outside)						/* TRUE if outside (counter-clockwise) contour */
{
	fix15 x, y;

#if DEBUG
	printf("BEGIN_CONTOUR_2(%3.1f, %3.1f, %s)\n",
		   (real) P1.x / (real) onepix, (real) P1.y / (real) onepix, outside ? "outside" : "inside");
#endif
	x = RANGECHECK(P1.x, sp_globals.xmin, sp_globals.xmax);
	y = RANGECHECK(P1.y, sp_globals.ymin, sp_globals.ymax);

	start_contour((fix31) x << sp_globals.poshift, (fix31) y << sp_globals.poshift, outside);
}
#endif

#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, curve2()
 * is called by curve() to output one curve segment.
 * If only one output module is included in the configuration, curve() is 
 * called by sp_proc_outl_data().
 * This function is only called when curve output is enabled.
 */
void sp_curve_outline(
	point_t P1,	/* First control point of Bezier curve */
	point_t P2,								/* Second control point of Bezier curve */
	point_t P3,								/* End point of Bezier curve */
	fix15 depth)
{
	fix15 x1, y1, x2, y2, x3, y3;

#if DEBUG
	printf("CURVE_2(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (real) P1.x / (real) onepix, (real) P1.y / (real) onepix,
		   (real) P2.x / (real) onepix, (real) P2.y / (real) onepix,
		   (real) P3.x / (real) onepix, (real) P3.y / (real) onepix);
#endif
	x1 = RANGECHECK(P1.x, sp_globals.xmin, sp_globals.xmax);
	y1 = RANGECHECK(P1.y, sp_globals.ymin, sp_globals.ymax);

	x2 = RANGECHECK(P2.x, sp_globals.xmin, sp_globals.xmax);
	y2 = RANGECHECK(P2.y, sp_globals.ymin, sp_globals.ymax);

	x3 = RANGECHECK(P3.x, sp_globals.xmin, sp_globals.xmax);
	y3 = RANGECHECK(P3.y, sp_globals.ymin, sp_globals.ymax);

	curve_to((fix31) x1 << sp_globals.poshift, (fix31) y1 << sp_globals.poshift,
			 (fix31) x2 << sp_globals.poshift, (fix31) y2 << sp_globals.poshift,
			 (fix31) x3 << sp_globals.poshift, (fix31) y3 << sp_globals.poshift);
}
#endif


#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, line2()
 * is called by line() to output one vector.
 * If only one output module is included in the configuration, line() is 
 * called by sp_proc_outl_data(). If curve output is enabled, line() is also
 * called by sp_split_curve().
 */
void sp_line_outline(point_t P1)	/* End point of vector */
{
	fix15 x1, y1;

#if DEBUG
	printf("LINE_2(%3.1f, %3.1f)\n", (real) P1.x / (real) onepix, (real) P1.y / (real) onepix);
#endif
	x1 = RANGECHECK(P1.x, sp_globals.xmin, sp_globals.xmax);
	y1 = RANGECHECK(P1.y, sp_globals.ymin, sp_globals.ymax);

	line_to((fix31) x1 << sp_globals.poshift, (fix31) y1 << sp_globals.poshift);
}
#endif

#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, end_contour2()
 * is called by end_contour() to signal the end of a contour.
 * If only one output module is included in the configuration, end_contour() is 
 * called by sp_proc_outl_data().
 */
void sp_end_contour_outline(void)
{
#if DEBUG
	printf("END_CONTOUR_2()\n");
#endif
	close_contour();
}
#endif


#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, end_sub_char2()
 * is called by end_sub_char() to signal the end of sub-character data.
 * If only one output module is included in the configuration, end_sub_char() is 
 * called by sp_make_comp_char().
 */
void sp_end_sub_char_outline(void)
{
#if DEBUG
	printf("END_SUB_CHAR_2()\n");
#endif
}
#endif


#if INCL_OUTLINE
/*
 * If two or more output modules are included in the configuration, end_char2()
 * is called by end_char() to signal the end of the character data.
 * If only one output module is included in the configuration, end_char() is 
 * called by sp_make_simp_char() and sp_make_comp_char().
 * Returns TRUE if output process is complete
 * Returns FALSE to repeat output of the transformed data beginning
 * with the first contour (of the first sub-char if compound).
 */
boolean sp_end_char_outline(void)
{
#if DEBUG
	printf("END_CHAR_2()\n");
#endif
	close_outline();
	return TRUE;
}
#endif