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


/*************************** O U T _ S C R N . C *****************************
 *                                                                           *
 * This is an output module for screen-writer mode.                          *
 *                                                                           *
 *****************************************************************************/

#include "linux/libcwrap.h"
#include "spdo_prv.h"					/* General definitions for Speedo   */
#if defined(__PUREC__) || defined(__mc68000__)
#include <mint/arch/nf_ops.h>
#endif

#if INCL_SCREEN /* whole file */

#define   DEBUG      0
#define   ABS(X)     ( (X < 0) ? -X : X)

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif


/*
 * init_out0() is called by sp_set_specs() to initialize the output module.
 * Returns TRUE if output module can accept requested specifications.
 * Returns FALSE otherwise.
 */
boolean sp_init_screen(specs_t *specsarg)
{
#if DEBUG
	printf("INIT_SCREEN()\n");
#endif
	UNUSED(specsarg);
	return TRUE;
}


/* Called once at the start of the character generation process
 */
boolean sp_begin_char_screen(fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy)
{
#if DEBUG
	printf("BEGIN_CHAR_SCREEN(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n",
		   (double) x / (double) sp_globals.onepix, (double) y / (double) sp_globals.onepix,
		   (double) minx / (double) sp_globals.onepix, (double) miny / (double) sp_globals.onepix,
		   (double) maxx / (double) sp_globals.onepix, (double) maxy / (double) sp_globals.onepix);
#endif
	if (sp_globals.pixshift > 8)
		sp_intercepts.fracpix = sp_globals.onepix << (8 - sp_globals.pixshift);
	else
		sp_intercepts.fracpix = sp_globals.onepix >> (sp_globals.pixshift - 8);

	sp_init_char_out(x, y, minx, miny, maxx, maxy);

#if defined(__PUREC__) || defined(__mc68000__)
	{
		static int done = 0;
		if (!done)
		{
			done = 1;
			nf_debugprintf("multshift: %d\n", sp_globals.multshift);
			nf_debugprintf("pixshift: %d\n", sp_globals.pixshift);
			nf_debugprintf("mpshift: %d\n", sp_globals.mpshift);
			nf_debugprintf("multrnd: $%lx\n", (long)sp_globals.multrnd);
			nf_debugprintf("pixrnd: $%x\n", sp_globals.pixrnd);
			nf_debugprintf("mprnd: $%lx\n", (long)sp_globals.mprnd);
			nf_debugprintf("onepix: $%x\n", sp_globals.onepix);
			nf_debugprintf("pixfix: $%x\n", sp_globals.pixfix);
			nf_debugprintf("fracpix: $%x\n", sp_globals.fracpix);
		}
	}
#endif
	return TRUE;
}


/*  Called by line() to add an intercept to the intercept list structure
 */
static void sp_add_intercept_screen(fix15 y,	/* Y coordinate in relative pixel units */
											/* (0 is lowest sample in band) */
											fix31 x)	/* X coordinate of intercept in subpixel units */
{
	fix15 from;				/* Insertion pointers for the linked list sort */
	fix15 to;
	fix15 xloc;
	fix15 xfrac;

#if DEBUG
	printf("    Add intercept(%2d, %x)\n", y + sp_globals.y_band.band_min, x);

	/* Bounds checking IS done in debug mode */
	if (y < 0)							/* Y value below bottom of current band? */
	{
		printf(" Intecerpt less than 0!!!\007\n");
		return;
	}

	if (y > (sp_globals.no_y_lists - 1))	/* Y value above top of current band? */
	{
		printf(" Intercept too big for band!!!!!\007\n");
		return;
	}
#endif

	/* Store new values */

	sp_intercepts.car[sp_globals.next_offset] = xloc = (fix15) (x >> 16);
	sp_intercepts.inttype[sp_globals.next_offset] = sp_intercepts.leftedge | (xfrac = ((x >> 8) & FRACTION));

	/* Find slot to insert new element (between from and to) */

	from = y;							/* Start at list head */

	while ((to = sp_intercepts.cdr[from]) != 0)	/* Until to == end of list */
	{
		if (xloc < sp_intercepts.car[to])	/* If next item is larger than or same as this one... */
			goto insert_element;		/* ... drop out and insert here */
		else if (xloc == sp_intercepts.car[to] && xfrac < (sp_intercepts.inttype[to] & FRACTION))
			goto insert_element;		/* ... drop out and insert here */
		from = to;						/* move forward in list */
	}

  insert_element:						/* insert element "sp_globals.next_offset" between elements "from" */
	/* and "to" */

	sp_intercepts.cdr[from] = sp_globals.next_offset;
	sp_intercepts.cdr[sp_globals.next_offset] = to;

	if (++sp_globals.next_offset >= MAX_INTERCEPTS)	/* Intercept buffer full? */
	{
		sp_globals.intercept_oflo = TRUE;
		/* There may be a few more calls to "add_intercept" from the current line */
		/* To avoid problems, we set next_offset to a safe value. We don't care   */
		/* if the intercept table gets trashed at this point                      */
		sp_globals.next_offset = sp_globals.first_offset;
	}
}


static void sp_vert_line_screen(fix31 x, fix15 y1, fix15 y2)
{

#ifdef DBGCRV
	printf("VERT_LINE_SCREEN(%6.4f, %6.4f, %6.4f)\n",
		   (double) (x - 32768L) / 65536.0, (double) (y1 - 32768L) / 65536.0, (double) (y2 - 32768L) / 65536.0);
#endif

	if (sp_globals.intercept_oflo)
		return;

	if (y1 > y2)						/* Line goes downwards ? */
	{
		if (y1 > (sp_globals.y_band.band_max + 1))	/* Start point above top of band? */
			y1 = sp_globals.y_band.band_max + 1;	/* Adjust start point to top of band */
		if (y2 < sp_globals.y_band.band_min)	/* End point below bottom of band? */
			y2 = sp_globals.y_band.band_min;	/* Adjust end point bottom of band */

		y1 -= sp_globals.y_band.band_min;	/* Translate start point to band origin */
		y2 -= sp_globals.y_band.band_min;	/* Translate end point to band origin */

		while (y2 < y1)					/* At least one intercept left? */
		{
			sp_add_intercept_screen(--y1, x);	/* Add intercept */
		}
	} else if (y2 > y1)					/* Line goes upwards ? */
	{
		if (y1 < sp_globals.y_band.band_min)	/* Start point below bottom of band? */
			y1 = sp_globals.y_band.band_min;	/* Adjust start point to bottom of band */
		if (y2 > (sp_globals.y_band.band_max + 1))	/* End point above top of band? */
			y2 = sp_globals.y_band.band_max + 1;	/* Adjust end point to top of band */

		y1 -= sp_globals.y_band.band_min;	/* Translate start point to band origin */
		y2 -= sp_globals.y_band.band_min;	/* Translate end point to band origin */

		while (y1 < y2)					/* At least one intercept left? */
		{
			sp_add_intercept_screen(y1++, x);	/* Add intercept */
		}
	}
}


/* Called for each curve in the transformed character if curves out enabled
 */
static void sp_scan_curve_screen(fix31 X0, fix31 Y0, fix31 X1, fix31 Y1, fix31 X2, fix31 Y2, fix31 X3, fix31 Y3)
{
	fix31 Pmidx;
	fix31 Pmidy;
	fix31 Pctrl1x;
	fix31 Pctrl1y;
	fix31 Pctrl2x;
	fix31 Pctrl2y;

#ifdef DBGCRV
	printf("SCAN_CURVE_SCREEN(%6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f)\n",
		   (double) (X0 - 32768L) / 65536.0, (double) (Y0 - 32768L) / 65536.0,
		   (double) (X1 - 32768L) / 65536.0, (double) (Y1 - 32768L) / 65536.0,
		   (double) (X2 - 32768L) / 65536.0, (double) (Y2 - 32768L) / 65536.0,
		   (double) (X3 - 32768L) / 65536.0, (double) (Y3 - 32768L) / 65536.0);
#endif

	if (((Y3 >> 16)) == (Y0 >> 16) || (Y3 + 1) == Y0 || Y3 == (Y0 + 1))
	{
		return;
	}
	if ((X3 >> 16) == (X0 >> 16))
	{
		sp_vert_line_screen(X3, (fix15) (Y0 >> 16), (fix15) (Y3 >> 16));
		return;
	}
	Pmidx = (X0 + (X1 + X2) * 3 + X3 + 4) >> 3;
	Pmidy = (Y0 + (Y1 + Y2) * 3 + Y3 + 4) >> 3;

	Pctrl1x = (X0 + X1 + 1) >> 1;
	Pctrl1y = (Y0 + Y1 + 1) >> 1;
	Pctrl2x = (X0 + (X1 << 1) + X2 + 2) >> 2;
	Pctrl2y = (Y0 + (Y1 << 1) + Y2 + 2) >> 2;
	sp_scan_curve_screen(X0, Y0, Pctrl1x, Pctrl1y, Pctrl2x, Pctrl2y, Pmidx, Pmidy);

	Pctrl1x = (X1 + (X2 << 1) + X3 + 2) >> 2;
	Pctrl1y = (Y1 + (Y2 << 1) + Y3 + 2) >> 2;
	Pctrl2x = (X2 + X3 + 1) >> 1;
	Pctrl2y = (Y2 + Y3 + 1) >> 1;
	sp_scan_curve_screen(Pmidx, Pmidy, Pctrl1x, Pctrl1y, Pctrl2x, Pctrl2y, X3, Y3);
}



/* Called at the start of each contour
 */
void sp_begin_contour_screen(fix31 x1, fix31 y1, boolean outside)
{
#if DEBUG
	printf("BEGIN_CONTOUR_SCREEN(%3.1f, %3.1f, %s)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix,
		   outside ? "outside" : "inside");
#endif
	UNUSED(outside);
	sp_globals.x0_spxl = x1;
	sp_globals.y0_spxl = y1;
	sp_globals.y_pxl = (sp_globals.y0_spxl + sp_globals.pixrnd) >> sp_globals.pixshift;
}


void sp_curve_screen(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth)
{
	fix31 X0;
	fix31 Y0;
	fix31 X1;
	fix31 Y1;
	fix31 X2;
	fix31 Y2;
	fix31 X3;
	fix31 Y3;

#if DEBUG
	printf("CURVE_SCREEN(%6.4f, %6.4f, %6.4f, %6.4f, %6.4f, %6.4f)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix,
		   (double) x2 / (double) sp_globals.onepix, (double) y2 / (double) sp_globals.onepix,
		   (double) x3 / (double) sp_globals.onepix, (double) y3 / (double) sp_globals.onepix);
#endif
	UNUSED(depth);

	if (sp_globals.extents_running)		/* Accumulate actual character extents if required */
	{
		if (x3 > sp_globals.bmap_xmax)
			sp_globals.bmap_xmax = x3;
		if (x3 < sp_globals.bmap_xmin)
			sp_globals.bmap_xmin = x3;
		if (y3 > sp_globals.bmap_ymax)
			sp_globals.bmap_ymax = y3;
		if (y3 < sp_globals.bmap_ymin)
			sp_globals.bmap_ymin = y3;
	}

	X0 = ((fix31) sp_globals.x0_spxl << sp_globals.poshift) + (fix31) 32768L;
	Y0 = ((fix31) sp_globals.y0_spxl << sp_globals.poshift) + (fix31) 32768L;
	X1 = ((fix31) x1 << sp_globals.poshift) + (fix31) 32768L;
	Y1 = ((fix31) y1 << sp_globals.poshift) + (fix31) 32768L;
	X2 = ((fix31) x2 << sp_globals.poshift) + (fix31) 32768L;
	Y2 = ((fix31) y2 << sp_globals.poshift) + (fix31) 32768L;
	X3 = ((fix31) x3 << sp_globals.poshift) + (fix31) 32768L;
	Y3 = ((fix31) y3 << sp_globals.poshift) + (fix31) 32768L;

	if (((Y0 - Y3) * sp_globals.tcb.mirror) > 0)
	{
		sp_intercepts.leftedge = LEFT_INT;
	} else
	{
		sp_intercepts.leftedge = 0;
	}

	sp_scan_curve_screen(X0, Y0, X1, Y1, X2, Y2, X3, Y3);
	sp_globals.x0_spxl = x3;
	sp_globals.y0_spxl = y3;
	sp_globals.y_pxl = (y3 + sp_globals.pixrnd) >> sp_globals.pixshift;	/* calculate new end-scan sp_globals.line */
}


/* Called for each vector in the transformed character
 */
void sp_line_screen(fix31 x1, fix31 y1)
{
	fix15 how_many_y;			/* # of intercepts at y = n + 1/2  */
	fix15 yc;					/* Current scan-line */
	fix15 temp1;						/* various uses */
	fix15 temp2;						/* various uses */
	fix31 dx_dy;				/* slope of line in 16.16 form */
	fix31 xc;					/* high-precision (16.16) x coordinate */
	fix15 x0, y0;				/* PIX.FRAC start and endpoints */

	x0 = sp_globals.x0_spxl;		/* get start of line (== current point) */
	y0 = sp_globals.y0_spxl;
	sp_globals.x0_spxl = x1;		/* end of line */
	sp_globals.y0_spxl = y1;		/*  (also update current point to end of line) */

	yc = sp_globals.y_pxl;				/* current scan line = end of last line */
	sp_globals.y_pxl = (y1 + sp_globals.pixrnd) >> sp_globals.pixshift;	/* calculate new end-scan sp_globals.line */


#if DEBUG
	printf("LINE_SCREEN(%3.4f, %3.4f)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix);
#endif

	if (sp_globals.extents_running)
	{
		if (sp_globals.x0_spxl > sp_globals.bmap_xmax)
			sp_globals.bmap_xmax = sp_globals.x0_spxl;
		if (sp_globals.x0_spxl < sp_globals.bmap_xmin)
			sp_globals.bmap_xmin = sp_globals.x0_spxl;
		if (sp_globals.y0_spxl > sp_globals.bmap_ymax)
			sp_globals.bmap_ymax = sp_globals.y0_spxl;
		if (sp_globals.y0_spxl < sp_globals.bmap_ymin)
			sp_globals.bmap_ymin = sp_globals.y0_spxl;
	}

	if (sp_globals.intercept_oflo)
		return;

	if ((how_many_y = sp_globals.y_pxl - yc) == 0)
		return;							/* Don't draw a null line */

	xc = (fix31) (x0 + sp_globals.pixrnd) << (16 - sp_globals.pixshift);	/* Original x coordinate with built in  */
	/* rounding. 16.16 form */

	if (how_many_y < 0)
	{
		yc--;							/* Predecrment downward lines */
	}

	if ((how_many_y * sp_globals.tcb.mirror) < 0)
	{
		sp_intercepts.leftedge = LEFT_INT;
	} else
	{
		sp_intercepts.leftedge = 0;
	}

	if (yc > sp_globals.y_band.band_max)	/* Is start point above band? */
	{
		if (sp_globals.y_pxl > sp_globals.y_band.band_max)
			return;						/* line has to go down! */
		how_many_y = sp_globals.y_pxl - (yc = sp_globals.y_band.band_max) - 1;	/* Yes, limit it */
	}

	if (yc < sp_globals.y_band.band_min)	/* Is start point below band? */
	{
		if (sp_globals.y_pxl < sp_globals.y_band.band_min)
			return;						/* line has to go up! */
		how_many_y = sp_globals.y_pxl - (yc = sp_globals.y_band.band_min);	/* Yes, limit it */
	}

	if ((temp1 = (x1 - x0)) == 0)		/* check for vertical line */
	{
		dx_dy = 0L;						/* Zero slope, leave xc alone */
		goto skip_calc;
	}

	/* calculate dx_dy at 16.16 fixed point */

	dx_dy = ((fix31) temp1 << 16) / (fix31) (y1 - y0);

	/* We have to check for a @#$%@# possible multiply overflow  */
	/* by doing another @#$*& multiply.  In assembly language,   */
	/* the program could just check the OVerflow flag or whatever*/
	/* works on the particular processor.  This C code is meant  */
	/* to be processor independant.                              */
	temp1 = (yc << sp_globals.pixshift) - y0 + sp_globals.pixrnd;

	/* This sees if the sign bits start at bit 15 */
	/* if they do, no overflow has occurred       */

	temp2 = (fix15) (MULT16(temp1, (fix15) (dx_dy >> 16)) >> 15);

	if ((temp2 != (fix15) - 1) && (temp2 != 0x0000))
	{									/* Overflow. Pick point closest to yc + .5 */
		if (ABS(temp1) < ABS((yc << sp_globals.pixshift) - y1 + sp_globals.pixrnd))
		{								/* use x1 instead of x0 */
			xc = (fix31) (x1 + sp_globals.pixrnd) << (16 - sp_globals.pixshift);
		}
		goto skip_calc;
	}

	/* calculate new xc at the center of the *current* scan line */
	/* due to banding, yc may be several lines away from y0      */
	/*  xc += (yc + .5 - y0) * dx_dy */
	/* This multiply generates a subpixel delta. */
	/* So we shift it to be a 16.16 delta */

	xc += ((fix31) temp1 * dx_dy) >> sp_globals.pixshift;

  skip_calc:

	yc -= sp_globals.y_band.band_min;	/* yc is now an offset relative to the band */

	if (how_many_y < 0)
	{									/* Vector down */
		if ((how_many_y += yc + 1) < 0)
			how_many_y = 0;				/* can't go below 0 */
		while (yc >= how_many_y)
		{
			sp_add_intercept_screen(yc--, xc);
			xc -= dx_dy;
		}
	} else
	{									/* Vector up */
		/* check to see that line doesn't extend beyond top of band */
		if ((how_many_y += yc) > sp_globals.no_y_lists)
			how_many_y = sp_globals.no_y_lists;
		while (yc != how_many_y)
		{
			sp_add_intercept_screen(yc++, xc);
			xc += dx_dy;
		}
	}
}


/* Called after the last vector in each contour
 */
void sp_end_contour_screen(void)
{
#if DEBUG
	printf("END_CONTOUR_SCREEN()\n");
#endif
	sp_intercepts.inttype[sp_globals.next_offset - 1] |= END_INT;
}



/*  Called by sp_make_char to output accumulated intercept lists
 *  Clips output to sp_globals.xmin, sp_globals.xmax, sp_globals.ymin, sp_globals.ymax boundaries
 */
static void sp_proc_intercepts_screen(void)
{
	fix15 i, j, jplus1, iminus1;
	fix15 k, nextk, previ;
	fix15 from, to;						/* Start and end of run in pixel units   
										   relative to left extent of character  */
	fix15 y;
	fix15 scan_line;

	fix15 first_y, last_y;
	fix15 xsave;

	fix15 diff;

#if DEBUG
	printf("\nPROC_INTERCEPTS_SCREEN: Intercept lists before:\n");
#endif

#if INCL_CLIPPING
	if ((sp_globals.specs.flags & CLIP_LEFT) != 0)
		clipleft = TRUE;
	else
		clipleft = FALSE;
	if ((sp_globals.specs.flags & CLIP_RIGHT) != 0)
		clipright = TRUE;
	else
		clipright = FALSE;
	if (clipleft || clipright)
	{
		xmax = sp_globals.clip_xmax + sp_globals.xmin;
		xmin = sp_globals.clip_xmin + sp_globals.xmin;
	}
	if (!clipright)
		xmax = ((sp_globals.set_width.x + 32768L) >> 16);
#endif

	if ((first_y = sp_globals.y_band.band_max) >= sp_globals.ymax)
		first_y = sp_globals.ymax - 1;	/* Clip to sp_globals.ymax boundary */

	if ((last_y = sp_globals.y_band.band_min) < sp_globals.ymin)
		last_y = sp_globals.ymin;		/* Clip to sp_globals.ymin boundary */

	last_y -= sp_globals.y_band.band_min;

#if DEBUG
	/* Print out all of the intercept info */
	scan_line = sp_globals.ymax - first_y - 1;

	for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
	{
		i = y;							/* Index head of intercept list */
		while ((i = sp_intercepts.cdr[i]) != 0)	/* Link to next intercept if present */
		{
			if ((from = sp_intercepts.car[i] - sp_globals.xmin) < 0)
				from = 0;				/* Clip to sp_globals.xmin boundary */
			i = sp_intercepts.cdr[i];	/* Link to next intercept */
			if (i == 0)					/* End of list? */
			{
				printf("****** proc_intercepts: odd number of intercepts\n");
				break;
			}
			if ((to = sp_intercepts.car[i]) > sp_globals.xmax)
				to = sp_globals.xmax - sp_globals.xmin;	/* Clip to sp_globals.xmax boundary */
			else
				to -= sp_globals.xmin;
			printf("    Y = %2d (scanline %2d): %d %d:\n", y + sp_globals.y_band.band_min, scan_line, from, to);
		}
	}
#endif

	/* CHECK INTERCEPT LIST FOR DROPOUT AND WINDING, FIX IF NECESSARY  */

	for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--)
	{
		previ = y;
		i = sp_intercepts.cdr[y];		/* Index head of intercept list */
		while (i != 0)					/* Link to next intercept if present */
		{
			j = i;
			i = sp_intercepts.cdr[i];	/* Link to next intercept */
			if (sp_intercepts.inttype[i] & LEFT_INT)
			{
				if (sp_intercepts.inttype[j] & LEFT_INT)
				{
					do
					{
						i = sp_intercepts.cdr[i];
					} while (sp_intercepts.inttype[i] & LEFT_INT);
					do
					{
						i = sp_intercepts.cdr[i];
					} while (sp_intercepts.cdr[i] && !(sp_intercepts.inttype[sp_intercepts.cdr[i]] & LEFT_INT));
					sp_intercepts.cdr[j] = i;
				} else
				{
					xsave = sp_intercepts.car[j];
					sp_intercepts.car[j] = sp_intercepts.car[i];
					sp_intercepts.car[i] = xsave;

					xsave = sp_intercepts.inttype[j];
					sp_intercepts.inttype[j] = sp_intercepts.inttype[i] & FRACTION;
					sp_intercepts.inttype[i] = xsave | LEFT_INT;

					sp_intercepts.cdr[previ] = i;
					sp_intercepts.cdr[j] = sp_intercepts.cdr[i];
					sp_intercepts.cdr[i] = j;
					i = j;
					j = sp_intercepts.cdr[previ];
				}
			}

			if (sp_intercepts.car[j] < sp_globals.xmin)
				sp_intercepts.car[j] = sp_globals.xmin;	/* Clip to sp_globals.xmin boundary */

			if (sp_intercepts.car[i] > sp_globals.xmax)
				sp_intercepts.car[i] = sp_globals.xmax;

			if (sp_intercepts.car[j] >= sp_intercepts.car[i])
			{
				if ((ufix16) (sp_intercepts.inttype[j] & FRACTION) + (ufix16) (sp_intercepts.inttype[i] & FRACTION) >
					sp_intercepts.fracpix)
					++sp_intercepts.car[i];
				else
					--sp_intercepts.car[j];
			}
			if (sp_globals.first_pass)
			{
				if (sp_intercepts.inttype[i - 1] & END_INT)
				{
					for (iminus1 = i + 1; !(sp_intercepts.inttype[iminus1] & END_INT); iminus1++)
						;
				} else
					iminus1 = i - 1;

				if (sp_intercepts.inttype[j] & END_INT)
				{
					for (jplus1 = j - 1; !(sp_intercepts.inttype[jplus1] & END_INT); jplus1--)
						;
					jplus1++;
				} else
					jplus1 = j + 1;

				if ((sp_intercepts.inttype[iminus1] & LEFT_INT))
				{
					if (sp_intercepts.car[jplus1] > sp_intercepts.car[i])
					{
						diff = sp_intercepts.car[jplus1] - sp_intercepts.car[i];
						sp_intercepts.car[i] += diff / 2;
						sp_intercepts.car[jplus1] -= diff / 2;
						if (diff & 1)
						{
							if ((ufix16) (sp_intercepts.inttype[i] & FRACTION) + (ufix16) (sp_intercepts.inttype[jplus1] & FRACTION) > sp_intercepts.fracpix)
								sp_intercepts.car[i]++;
							else
								sp_intercepts.car[jplus1]--;
						}
					}
				} else if (!(sp_intercepts.inttype[jplus1] & LEFT_INT))
				{
					if (sp_intercepts.car[iminus1] < sp_intercepts.car[j])
					{
						diff = sp_intercepts.car[j] - sp_intercepts.car[iminus1];
						sp_intercepts.car[j] -= diff / 2;
						sp_intercepts.car[iminus1] += diff / 2;
						if (diff & 1)
						{
							if ((ufix16) (sp_intercepts.inttype[j] & FRACTION) +
								(ufix16) (sp_intercepts.inttype[iminus1] & FRACTION) > sp_intercepts.fracpix)
								sp_intercepts.car[iminus1]++;
							else
								sp_intercepts.car[j]--;
						}
					}
				}
				if (sp_globals.tcb.mirror == -1)
				{
					if (sp_intercepts.inttype[j - 1] & END_INT)
					{
						for (jplus1 = j + 1; !(sp_intercepts.inttype[jplus1] & END_INT); jplus1++)
							;
					} else
					{
						jplus1 = j - 1;
					}
				}

				if (!(sp_intercepts.inttype[jplus1] & LEFT_INT) && sp_intercepts.car[j] > sp_intercepts.car[jplus1])
				{
					k = sp_intercepts.cdr[y - 1];
					while (k > 0)
					{
						nextk = sp_intercepts.cdr[k];
						if (!(sp_intercepts.inttype[k] & LEFT_INT) &&
							(sp_intercepts.inttype[nextk] & LEFT_INT) &&
							sp_intercepts.car[nextk] > sp_intercepts.car[jplus1])
						{
							if ((diff = sp_intercepts.car[j] - sp_intercepts.car[k]) > 0)
							{
								if (diff <= (sp_intercepts.car[nextk] - sp_intercepts.car[jplus1]))
								{
									sp_intercepts.car[j] -= diff / 2;
									sp_intercepts.car[k] += diff / 2;
									if (diff & 1)
									{
										if ((ufix16) (sp_intercepts.inttype[j] & FRACTION) +
											(ufix16) (sp_intercepts.inttype[k] & FRACTION) > sp_intercepts.fracpix)
											sp_intercepts.car[j]--;
										else
											sp_intercepts.car[k]++;
									}
								} else
								{
									diff = sp_intercepts.car[nextk] - sp_intercepts.car[jplus1];
									sp_intercepts.car[nextk] -= diff / 2;
									sp_intercepts.car[jplus1] += diff / 2;
									if (diff & 1)
									{
										if ((ufix16) (sp_intercepts.inttype[jplus1] & FRACTION) +
											(ufix16) (sp_intercepts.inttype[nextk] & FRACTION) > sp_intercepts.fracpix)
											sp_intercepts.car[nextk]--;
										else
											sp_intercepts.car[jplus1]++;
									}
								}
							}
							break;
						}
						k = nextk;
					}
				}
				if (j > 0 && sp_intercepts.car[j - 1] > sp_intercepts.car[i]
					&& !(sp_intercepts.inttype[j - 1] & END_INT))
				{
					diff = sp_intercepts.car[j - 1] - sp_intercepts.car[i];
					sp_intercepts.car[i] += diff / 2;
					sp_intercepts.car[j - 1] -= diff / 2;
					if (diff & 1)
					{
						if ((ufix16) (sp_intercepts.inttype[i] & FRACTION) +
							(ufix16) (sp_intercepts.inttype[j - 1] & FRACTION) > sp_intercepts.fracpix)
							sp_intercepts.car[i]++;
						else
							sp_intercepts.car[j - 1]--;
					}
				}
				if (sp_intercepts.car[i + 1] < sp_intercepts.car[j] && !(sp_intercepts.inttype[i] & END_INT))
				{
					diff = sp_intercepts.car[j] - sp_intercepts.car[i + 1];
					sp_intercepts.car[j] -= diff / 2;
					sp_intercepts.car[i + 1] += diff / 2;
					if (diff & 1)
					{
						if ((ufix16) (sp_intercepts.inttype[j] & FRACTION) +
							(ufix16) (sp_intercepts.inttype[i + 1] & FRACTION) > sp_intercepts.fracpix)
							sp_intercepts.car[i + 1]++;
						else
							sp_intercepts.car[j]--;
					}

				}
				previ = i;
			}
			i = sp_intercepts.cdr[i];
		}
	}

#if DEBUG
	printf("\nPROC_INTERCEPTS_SCREEN: Intercept lists after:\n");
	/* Print out all of the intercept info */
	scan_line = sp_globals.ymax - first_y - 1;

	for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
	{
		i = y;							/* Index head of intercept list */
		while ((i = sp_intercepts.cdr[i]) != 0)	/* Link to next intercept if present */
		{
			if ((from = sp_intercepts.car[i] - sp_globals.xmin) < 0)
				from = 0;				/* Clip to sp_globals.xmin boundary */
			i = sp_intercepts.cdr[i];	/* Link to next intercept */
			if (i == 0)					/* End of list? */
			{
				printf("****** proc_intercepts: odd number of intercepts\n");
				break;
			}
			if ((to = sp_intercepts.car[i]) > sp_globals.xmax)
				to = sp_globals.xmax - sp_globals.xmin;	/* Clip to sp_globals.xmax boundary */
			else
				to -= sp_globals.xmin;
			printf("    Y = %2d (scanline %2d): %d %d:\n", y + sp_globals.y_band.band_min, scan_line, from, to);
		}
	}
#endif

	/*  INTERCEPTS ALL PATCHED, NOW DRAW THE IMAGE */
	scan_line = sp_globals.ymax - first_y - 1;

	for (y = first_y - sp_globals.y_band.band_min; y >= last_y; y--, scan_line++)
	{
		i = sp_intercepts.cdr[y];		/* Index head of intercept list */
		while (i != 0)					/* Link to next intercept if present */
		{
			from = sp_intercepts.car[i];
			i = sp_intercepts.cdr[i];	/* Link to next intercept */
			to = sp_intercepts.car[i];
#if INCL_CLIPPING
			if (clipleft)
			{
				if (to <= xmin)
				{
					i = sp_intercepts.cdr[i];
					continue;
				}
				if (from < xmin)
					from = xmin;
			}
			if (clipright)
			{
				if (from >= xmax)
				{
					i = sp_intercepts.cdr[i];
					continue;
				}
				if (to > xmax)
					to = xmax;
			}
#endif
			set_bitmap_bits(scan_line, from - sp_globals.xmin, to - sp_globals.xmin);
			i = sp_intercepts.cdr[i];
		}
	}
}


/* Called when all character data has been output
 * Return TRUE if output process is complete
 * Return FALSE to repeat output of the transformed data beginning
 * with the first contour
 */
boolean sp_end_char_screen(void)
{
	fix31 xorg;
	fix31 yorg;

#if INCL_CLIPPING
	fix31 em_max, em_min, bmap_max, bmap_min;
#endif

#if DEBUG
	printf("END_CHAR_SCREEN()\n");
#endif

	if (sp_globals.first_pass)
	{
		if (sp_globals.bmap_xmax >= sp_globals.bmap_xmin)
		{
			sp_globals.xmin = (sp_globals.bmap_xmin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
			sp_globals.xmax = (sp_globals.bmap_xmax + sp_globals.pixrnd) >> sp_globals.pixshift;
		} else
		{
			sp_globals.xmin = sp_globals.xmax = 0;
		}
		if (sp_globals.bmap_ymax >= sp_globals.bmap_ymin)
		{

#if INCL_CLIPPING
			switch (sp_globals.tcb0.xtype)
			{
			case 1:					/* 180 degree rotation */
				if (sp_globals.specs.flags & CLIP_TOP)
				{
					sp_globals.clip_ymin = ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_ymin = sp_globals.clip_ymin >> sp_globals.multshift;
					bmap_min = (sp_globals.bmap_ymin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
					sp_globals.clip_ymin = -1 * sp_globals.clip_ymin;
					if (bmap_min < sp_globals.clip_ymin)
						sp_globals.ymin = sp_globals.clip_ymin;
					else
						sp_globals.ymin = bmap_min;
				}
				if (sp_globals.specs.flags & CLIP_BOTTOM)
				{
					sp_globals.clip_ymax = ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_ymax = sp_globals.clip_ymax >> sp_globals.multshift;
					bmap_max = (sp_globals.bmap_ymax + sp_globals.pixrnd) >> sp_globals.pixshift;
					if (bmap_max < sp_globals.clip_ymax)
						sp_globals.ymax = bmap_max;
					else
						sp_globals.ymax = sp_globals.clip_ymax;
				}
				sp_globals.clip_xmax = -sp_globals.xmin;
				sp_globals.clip_xmin = ((sp_globals.set_width.x + 32768L) >> 16) - sp_globals.xmin;
				break;
			case 2:					/* 90 degree rotation */
				if (sp_globals.specs.flags & CLIP_TOP)
				{
					sp_globals.clip_xmin = ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_xmin = sp_globals.clip_xmin >> sp_globals.multshift;
					sp_globals.clip_xmin = -1 * sp_globals.clip_xmin;
					bmap_min = (sp_globals.bmap_xmin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
					if (bmap_min > sp_globals.clip_xmin)
						sp_globals.clip_xmin = bmap_min;

					/* normalize to x origin */
					sp_globals.clip_xmin -= sp_globals.xmin;
				}
				if (sp_globals.specs.flags & CLIP_BOTTOM)
				{
					sp_globals.clip_xmax =
						(fix31) ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_xmax = sp_globals.clip_xmax >> sp_globals.multshift;
					bmap_max = (sp_globals.bmap_xmax + sp_globals.pixrnd) >> sp_globals.pixshift;
					if (bmap_max < sp_globals.clip_xmax)
						sp_globals.xmax = bmap_max;
					else
						sp_globals.xmax = sp_globals.clip_xmax;
					sp_globals.clip_ymax = 0;
					if ((sp_globals.specs.flags & CLIP_TOP) && (sp_globals.ymax > sp_globals.clip_ymax))
						sp_globals.ymax = sp_globals.clip_ymax;
					sp_globals.clip_ymin = ((sp_globals.set_width.y + 32768L) >> 16);
					if ((sp_globals.specs.flags & CLIP_BOTTOM) && (sp_globals.ymin < sp_globals.clip_ymin))
						sp_globals.ymin = sp_globals.clip_ymin;
					/* normalize to x origin */
					sp_globals.clip_xmax -= sp_globals.xmin;
				}
				break;
			case 3:					/* 270 degree rotation */
				if (sp_globals.specs.flags & CLIP_TOP)
				{
					sp_globals.clip_xmin = ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_xmin = sp_globals.clip_xmin >> sp_globals.multshift;
					sp_globals.clip_xmin = -1 * sp_globals.clip_xmin;
					bmap_min = (sp_globals.bmap_xmin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;

					/* let the minimum be the larger of these two values */
					if (bmap_min > sp_globals.clip_xmin)
						sp_globals.clip_xmin = bmap_min;

					/* normalize the x value to new xorgin */
					sp_globals.clip_xmin -= sp_globals.xmin;
				}
				if (sp_globals.specs.flags & CLIP_BOTTOM)
				{
					sp_globals.clip_xmax = ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_xmax = sp_globals.clip_xmax >> sp_globals.multshift;
					bmap_max = (sp_globals.bmap_xmax + sp_globals.pixrnd) >> sp_globals.pixshift;

					/* let the max be the lesser of these two values */
					if (bmap_max < sp_globals.clip_xmax)
					{
						sp_globals.xmax = bmap_max;
						sp_globals.clip_xmax = bmap_max;
					} else
						sp_globals.xmax = sp_globals.clip_xmax;

					/* normalize the x value to new x origin */
					sp_globals.clip_xmax -= sp_globals.xmin;
				}
				/* compute y clip values */
				sp_globals.clip_ymax = ((sp_globals.set_width.y + 32768L) >> 16);
				if ((sp_globals.specs.flags & CLIP_TOP) && (sp_globals.ymax > sp_globals.clip_ymax))
					sp_globals.ymax = sp_globals.clip_ymax;
				sp_globals.clip_ymin = 0;
				if ((sp_globals.specs.flags & CLIP_BOTTOM) && (sp_globals.ymin < sp_globals.clip_ymin))
					sp_globals.ymin = sp_globals.clip_ymin;
				break;
			default:					/* this is for zero degree rotation and arbitrary rotation */
				if (sp_globals.specs.flags & CLIP_TOP)
				{
					sp_globals.clip_ymax = ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_ymax = sp_globals.clip_ymax >> sp_globals.multshift;
					bmap_max = (sp_globals.bmap_ymax + sp_globals.pixrnd) >> sp_globals.pixshift;
					if (bmap_max > sp_globals.clip_ymax)
						sp_globals.ymax = bmap_max;
					else
						sp_globals.ymax = sp_globals.clip_ymax;
				}
				if (sp_globals.specs.flags & CLIP_BOTTOM)
				{
					sp_globals.clip_ymin = ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
					sp_globals.clip_ymin = sp_globals.clip_ymin >> sp_globals.multshift;
					sp_globals.clip_ymin = -sp_globals.clip_ymin;
					bmap_min = (sp_globals.bmap_ymin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
					if (bmap_min < sp_globals.clip_ymin)
						sp_globals.ymin = sp_globals.clip_ymin;
					else
						sp_globals.ymin = bmap_min;
				}
				sp_globals.clip_xmin = -sp_globals.xmin;
				sp_globals.clip_xmax = ((sp_globals.set_width.x + 32768L) >> 16) - sp_globals.xmin;
				break;
			}
			if (!(sp_globals.specs.flags & CLIP_TOP))
#endif
				sp_globals.ymax = (sp_globals.bmap_ymax + sp_globals.pixrnd) >> sp_globals.pixshift;

#if INCL_CLIPPING
			if (!(sp_globals.specs.flags & CLIP_BOTTOM))
#endif

				sp_globals.ymin = (sp_globals.bmap_ymin + sp_globals.pixrnd + 1) >> sp_globals.pixshift;
		} else
		{
			sp_globals.ymin = sp_globals.ymax = 0;
		}

		/* add in the rounded out part (from xform.) of the left edge */
		if (sp_globals.tcb.xmode == 0)	/* for X pix is function of X orus only add the round */
			xorg = (((fix31) sp_globals.xmin << 16) + (sp_globals.rnd_xmin << sp_globals.poshift));
		else if (sp_globals.tcb.xmode == 1)	/* for X pix is function of -X orus only, subtr. round */
			xorg = (((fix31) sp_globals.xmin << 16) - (sp_globals.rnd_xmin << sp_globals.poshift));
		else
			xorg = (fix31) sp_globals.xmin << 16;	/* for other cases don't use round on x */

		if (sp_globals.tcb.ymode == 2)	/* for Y pix is function of X orus only, add round error */
			yorg = (((fix31) sp_globals.ymin << 16) + (sp_globals.rnd_xmin << sp_globals.poshift));
		else if (sp_globals.tcb.ymode == 3)	/* for Y pix is function of -X orus only, sub round */
			yorg = (((fix31) sp_globals.ymin << 16) - (sp_globals.rnd_xmin << sp_globals.poshift));
		else							/* all other cases have no round error on yorg */
			yorg = (fix31) sp_globals.ymin << 16;

		open_bitmap(xorg, yorg, sp_globals.xmax - sp_globals.xmin, sp_globals.ymax - sp_globals.ymin);
		if (sp_globals.intercept_oflo)
		{
			sp_globals.y_band.band_min = sp_globals.ymin;
			sp_globals.y_band.band_max = sp_globals.ymax;
			sp_init_intercepts_out();
			sp_globals.first_pass = FALSE;
			sp_globals.extents_running = FALSE;
			return FALSE;
		} else
		{
			sp_proc_intercepts_screen();
			close_bitmap();
			return TRUE;
		}
	} else
	{
		if (sp_globals.intercept_oflo)
		{
			sp_reduce_band_size_out();
			sp_init_intercepts_out();
			return FALSE;
		} else
		{
			sp_proc_intercepts_screen();
			if (sp_next_band_out())
			{
				sp_init_intercepts_out();
				return FALSE;
			}
			close_bitmap();
			return TRUE;
		}
	}
}

#else

extern int _I_dont_care_that_ISO_C_forbids_an_empty_source_file_;

#endif /* INCL_SCREEN */
