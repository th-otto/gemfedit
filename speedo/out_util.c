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


/*************************** O U T _ U T I L . C *****************************
 *                                                                           *
 * This is a utility module share by all bitmap output modules               *
 *                                                                           *
 *****************************************************************************/


#include "linux/libcwrap.h"
#include "spdo_prv.h"					/* General definitions for Speedo   */

#define	DEBUG	0

/* absolute value function */
#define   ABS(X)     ( (X < 0) ? -X : X)

#if INCL_BLACK || INCL_2D || INCL_SCREEN

void sp_init_char_out(fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy)
{
	sp_globals.set_width.x = (fix31) x << sp_globals.poshift;
	sp_globals.set_width.y = (fix31) y << sp_globals.poshift;
	sp_set_first_band_out(minx, miny, maxx, maxy);
	sp_init_intercepts_out();
	if (sp_globals.normal)
	{
		sp_globals.bmap_xmin = minx;
		sp_globals.bmap_xmax = maxx;
		sp_globals.bmap_ymin = miny;
		sp_globals.bmap_ymax = maxy;
		sp_globals.extents_running = FALSE;
	} else
	{
		sp_globals.bmap_xmin = 32000;
		sp_globals.bmap_xmax = -32000;
		sp_globals.bmap_ymin = 32000;
		sp_globals.bmap_ymax = -32000;
		sp_globals.extents_running = TRUE;
	}
	sp_globals.first_pass = TRUE;
}

/* Called at the start of each sub-character in a composite character
 */
void sp_begin_sub_char_out(fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy)
{
#if DEBUG
	printf("BEGIN_SUB_CHAR_out(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f\n",
		   (real) x / (real) sp_globals.onepix, (real) y / (real) sp_globals.onepix,
		   (real) minx / (real) sp_globals.onepix, (real) miny / (real) sp_globals.onepix,
		   (real) maxx / (real) sp_globals.onepix, (real) maxy / (real) sp_globals.onepix);
#endif
	UNUSED(x);
	UNUSED(y);
	UNUSED(minx);
	UNUSED(miny);
	UNUSED(maxx);
	UNUSED(maxy);
	sp_restart_intercepts_out();
	if (!sp_globals.extents_running)
	{
		sp_globals.bmap_xmin = 32000;
		sp_globals.bmap_xmax = -32000;
		sp_globals.bmap_ymin = 32000;
		sp_globals.bmap_ymax = -32000;
		sp_globals.extents_running = TRUE;
	}
}

/* Called for each curve in the transformed character if curves out enabled
 */
void sp_curve_out(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth)
{
#if DEBUG
	printf("CURVE_OUT(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (real) x1 / (real) sp_globals.onepix, (real) y1 / (real) sp_globals.onepix,
		   (real) x2 / (real) sp_globals.onepix, (real) y2 / (real) sp_globals.onepix,
		   (real) x3 / (real) sp_globals.onepix, (real) y3 / (real) sp_globals.onepix);
#endif
	UNUSED(x1); UNUSED(y1);
	UNUSED(x2); UNUSED(y2);
	UNUSED(x3); UNUSED(y3);
	UNUSED(depth);
}



/* Called after the last vector in each contour
 */
void sp_end_contour_out(void)
{
#if DEBUG
	printf("END_CONTOUR_OUT()\n");
#endif
}



/* Called after the last contour in each sub-character in a compound character
 */
void sp_end_sub_char_out(void)
{
#if DEBUG
	printf("END_SUB_CHAR_OUT()\n");
#endif
}


/*  Called to initialize intercept storage data structure
 */
void sp_init_intercepts_out(void)
{
	fix15 i;
	fix15 no_lists;

#if DEBUG
	printf("    Init intercepts (Y band from %d to %d)\n", sp_globals.y_band.band_min, sp_globals.y_band.band_max);
	if (sp_globals.x_scan_active)
		printf("                    (X band from %d to %d)\n", sp_globals.x_band.band_min, sp_globals.x_band.band_max);
#endif

	sp_globals.intercept_oflo = FALSE;

	sp_globals.no_y_lists = sp_globals.y_band.band_max - sp_globals.y_band.band_min + 1;
#if INCL_2D
	if (sp_globals.output_mode == MODE_2D)
	{
		sp_globals.no_x_lists = sp_globals.x_scan_active ?
			sp_globals.x_band.band_max - sp_globals.x_band.band_min + 1 : 0;
		no_lists = sp_globals.no_y_lists + sp_globals.no_x_lists;
	} else
#endif
		no_lists = sp_globals.no_y_lists;

#if INCL_2D
	sp_globals.y_band.band_floor = 0;
	sp_globals.y_band.band_ceiling = sp_globals.no_y_lists;
#endif

	if (no_lists >= MAX_INTERCEPTS)		/* Not enough room for list table? */
	{
		no_lists = sp_globals.no_y_lists = MAX_INTERCEPTS;
		sp_globals.intercept_oflo = TRUE;
		sp_globals.y_band.band_min = sp_globals.y_band.band_max - sp_globals.no_y_lists + 1;
#if INCL_2D
		sp_globals.y_band.band_array_offset = sp_globals.y_band.band_min;
		sp_globals.y_band.band_ceiling = sp_globals.no_y_lists;
		sp_globals.no_x_lists = 0;
		sp_globals.x_scan_active = FALSE;
#endif
	}

	for (i = 0; i < no_lists; i++)		/* For each active value... */
	{
#if INCL_SCREEN
		if (sp_globals.output_mode == MODE_SCREEN)
			sp_intercepts.inttype[i] = 0;
#endif
		sp_intercepts.cdr[i] = 0;		/* Mark each intercept list empty */
	}

	sp_globals.first_offset = sp_globals.next_offset = no_lists;

#if INCL_2D
	sp_globals.y_band.band_array_offset = sp_globals.y_band.band_min;
	sp_globals.x_band.band_array_offset = sp_globals.x_band.band_min - sp_globals.no_y_lists;
	sp_globals.x_band.band_floor = sp_globals.no_y_lists;
	sp_globals.x_band.band_ceiling = no_lists;
#endif
#if INCL_SCREEN
	sp_intercepts.inttype[sp_globals.no_y_lists - 1] = END_INT;
#endif

}


/*  Called by sp_make_char when a new sub character is started
 *  Freezes current sorted lists
 */
void sp_restart_intercepts_out(void)
{

#if DEBUG
	printf("    Restart intercepts:\n");
#endif
	sp_globals.first_offset = sp_globals.next_offset;
}



void sp_set_first_band_out(fix31 minx, fix31 miny, fix31 maxx, fix31 maxy)
{
	sp_globals.ymin = miny;
	sp_globals.ymax = maxy;

	sp_globals.ymin = (sp_globals.ymin - sp_globals.onepix + 1) >> sp_globals.pixshift;
	sp_globals.ymax = (sp_globals.ymax + sp_globals.onepix - 1) >> sp_globals.pixshift;

#if INCL_CLIPPING
	switch (sp_globals.tcb0.xtype)
	{
	case 1:							/* 180 degree rotation */
		if (sp_globals.specs.flags & CLIP_TOP)
		{
			sp_globals.clip_ymin = (fix31) ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
			sp_globals.clip_ymin = sp_globals.clip_ymin >> sp_globals.multshift;
			sp_globals.clip_ymin = -1 * sp_globals.clip_ymin;
			if (sp_globals.ymin < sp_globals.clip_ymin)
				sp_globals.ymin = sp_globals.clip_ymin;
		}
		if (sp_globals.specs.flags & CLIP_BOTTOM)
		{
			sp_globals.clip_ymax =
				(fix31) ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
			sp_globals.clip_ymax = sp_globals.clip_ymax >> sp_globals.multshift;
			if (sp_globals.ymax > sp_globals.clip_ymax)
				sp_globals.ymax = sp_globals.clip_ymax;
		}
		break;
	case 2:							/* 90 degree rotation */
		sp_globals.clip_ymax = 0;
		if ((sp_globals.specs.flags & CLIP_TOP) && (sp_globals.ymax > sp_globals.clip_ymax))
			sp_globals.ymax = sp_globals.clip_ymax;
		sp_globals.clip_ymin = ((sp_globals.set_width.y + 32768L) >> 16);
		if ((sp_globals.specs.flags & CLIP_BOTTOM) && (sp_globals.ymin < sp_globals.clip_ymin))
			sp_globals.ymin = sp_globals.clip_ymin;
		break;
	case 3:							/* 270 degree rotation */
		sp_globals.clip_ymax = ((sp_globals.set_width.y + 32768L) >> 16);
		if ((sp_globals.specs.flags & CLIP_TOP) && (sp_globals.ymax > sp_globals.clip_ymax))
			sp_globals.ymax = sp_globals.clip_ymax;
		sp_globals.clip_ymin = 0;
		if ((sp_globals.specs.flags & CLIP_BOTTOM) && (sp_globals.ymin < sp_globals.clip_ymin))
			sp_globals.ymin = sp_globals.clip_ymin;
		break;
	default:							/* this is for zero degree rotation and arbitrary rotation */
		if (sp_globals.specs.flags & CLIP_TOP)
		{
			sp_globals.clip_ymax = (fix31) ((fix31) EM_TOP * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
			sp_globals.clip_ymax = sp_globals.clip_ymax >> sp_globals.multshift;
			if (sp_globals.ymax > sp_globals.clip_ymax)
				sp_globals.ymax = sp_globals.clip_ymax;
		}
		if (sp_globals.specs.flags & CLIP_BOTTOM)
		{
			sp_globals.clip_ymin =
				(fix31) ((fix31) (-1 * EM_BOT) * sp_globals.tcb0.yppo + ((1 << sp_globals.multshift) / 2));
			sp_globals.clip_ymin = sp_globals.clip_ymin >> sp_globals.multshift;
			sp_globals.clip_ymin = -sp_globals.clip_ymin;
			if (sp_globals.ymin < sp_globals.clip_ymin)
				sp_globals.ymin = sp_globals.clip_ymin;
		}
		break;
	}
#endif
	sp_globals.y_band.band_min = sp_globals.ymin;
	sp_globals.y_band.band_max = sp_globals.ymax - 1;

	sp_globals.xmin = (minx + sp_globals.pixrnd) >> sp_globals.pixshift;
	sp_globals.xmax = (maxx + sp_globals.pixrnd) >> sp_globals.pixshift;


#if INCL_2D
	sp_globals.x_band.band_min = sp_globals.xmin - 1;	/* subtract one pixel of "safety margin" */
	sp_globals.x_band.band_max = sp_globals.xmax /* - 1 + 1 */ ;	/* Add one pixel of "safety margin" */
#endif
}







void sp_reduce_band_size_out(void)
{
	sp_globals.y_band.band_min =
		sp_globals.y_band.band_max - ((sp_globals.y_band.band_max - sp_globals.y_band.band_min) >> 1);
#if INCL_2D
	sp_globals.y_band.band_array_offset = sp_globals.y_band.band_min;
#endif
}


boolean sp_next_band_out(void)
{
	fix15 tmpfix15;

	if (sp_globals.y_band.band_min <= sp_globals.ymin)
		return FALSE;
	tmpfix15 = sp_globals.y_band.band_max - sp_globals.y_band.band_min;
	sp_globals.y_band.band_max = sp_globals.y_band.band_min - 1;
	sp_globals.y_band.band_min = sp_globals.y_band.band_max - tmpfix15;
	if (sp_globals.y_band.band_min < sp_globals.ymin)
		sp_globals.y_band.band_min = sp_globals.ymin;
#if INCL_2D
	sp_globals.y_band.band_array_offset = sp_globals.y_band.band_min;
#endif
	return TRUE;
}
#endif
