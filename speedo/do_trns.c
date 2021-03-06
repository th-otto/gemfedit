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

/**************************** D O _ T R N S . C ******************************
 *                                                                           *
 * This module is responsible for executing all intelligent transformation   *
 * for bounding box and outline data                                         *
 *                                                                           *
 ****************************************************************************/


#include "linux/libcwrap.h"
#include "spdo_prv.h"					/* General definitions for Speedo    */

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif


/*
 * Called by sp_read_bbox() and sp_proc_outl_data() to read an X Y argument
 * pair from the font.
 * The format is specified as follows:
 *     Bits 0-1: Type of X argument.
 *     Bits 2-3: Type of Y argument.
 * where the 4 possible argument types are:
 *     Type 0:   Controlled coordinate represented by one byte
 *               index into the X or Y controlled coordinate table.
 *     Type 1:   Interpolated coordinate represented by a two-byte
 *               signed integer.
 *     Type 2:   Interpolated coordinate represented by a one-byte
 *               signed increment/decrement relative to the 
 *               proceding X or Y coordinate.
 *     Type 3:   Repeat of preceding X or Y argument value and type.
 * The units of P are sub-pixels.
 * Updates *ppointer to point to the byte following the
 * argument pair.
 */
static ufix8 *sp_get_args(SPD_PROTO_DECL2 ufix8 *pointer,	/* Pointer to next byte in char data */
										   ufix8 format,	/* Format specifiaction of argument pair */
										   point_t * pP)	/* Resulting transformed point */
{
	ufix8 edge;

	/* Read X argument */
	switch (format & 0x03)
	{
	case 0:							/* Index to controlled oru */
		edge = NEXT_BYTE(pointer);
		sp_globals.x_orus = sp_plaid.orus[edge];
#if INCL_RULES
		sp_globals.x_pix = sp_plaid.pix[edge];
#endif
		break;

	case 1:							/* 2 byte interpolated oru value */
		sp_globals.x_orus = NEXT_WORD(pointer);
		goto L1;

	case 2:							/* 1 byte signed oru increment */
		sp_globals.x_orus += (fix15) ((fix7) NEXT_BYTE(pointer));
	  L1:
#if INCL_RULES
		sp_globals.x_pix =
			TRANS(sp_globals.x_orus, sp_plaid.mult[sp_globals.x_int], sp_plaid.offset[sp_globals.x_int],
				  sp_globals.mpshift);
#endif
		break;

	default:							/* No change in X value */
		break;
	}

	/* Read Y argument */
	switch ((format >> 2) & 0x03)
	{
	case 0:							/* Index to controlled oru */
		edge = sp_globals.Y_edge_org + NEXT_BYTE(pointer);
		sp_globals.y_orus = sp_plaid.orus[edge];
#if INCL_RULES
		sp_globals.y_pix = sp_plaid.pix[edge];
#endif
		break;

	case 1:							/* 2 byte interpolated oru value */
		sp_globals.y_orus = NEXT_WORD(pointer);
		goto L2;

	case 2:							/* 1 byte signed oru increment */
		sp_globals.y_orus += (fix15) ((fix7) NEXT_BYTE(pointer));
	  L2:
#if INCL_RULES
		sp_globals.y_pix =
			TRANS(sp_globals.y_orus, sp_plaid.mult[sp_globals.y_int], sp_plaid.offset[sp_globals.y_int],
				  sp_globals.mpshift);
#endif
		break;

	default:							/* No change in X value */
		break;
	}

#if INCL_RULES
	switch (sp_globals.tcb.xmode)
	{
	case 0:							/* X mode 0 */
		pP->x = sp_globals.x_pix;
		break;

	case 1:							/* X mode 1 */
		pP->x = -sp_globals.x_pix;
		break;

	case 2:							/* X mode 2 */
		pP->x = sp_globals.y_pix;
		break;

	case 3:							/* X mode 3 */
		pP->x = -sp_globals.y_pix;
		break;

	default:							/* X mode 4 */
#endif
		pP->x = (MULT16(sp_globals.x_orus, sp_globals.tcb.xxmult) +
				 MULT16(sp_globals.y_orus, sp_globals.tcb.xymult) + sp_globals.tcb.xoffset) >> sp_globals.mpshift;
#if INCL_RULES
		break;
	}

	switch (sp_globals.tcb.ymode)
	{
	case 0:							/* Y mode 0 */
		pP->y = sp_globals.y_pix;
		break;

	case 1:							/* Y mode 1 */
		pP->y = -sp_globals.y_pix;
		break;

	case 2:							/* Y mode 2 */
		pP->y = sp_globals.x_pix;
		break;

	case 3:							/* Y mode 3 */
		pP->y = -sp_globals.x_pix;
		break;

	default:							/* Y mode 4 */
#endif
		pP->y = (MULT16(sp_globals.x_orus, sp_globals.tcb.yxmult) +
				 MULT16(sp_globals.y_orus, sp_globals.tcb.yymult) + sp_globals.tcb.yoffset) >> sp_globals.mpshift;
#if INCL_RULES
		break;
	}
#endif

	return pointer;
}


/*
 * Called by sp_make_simp_char() and sp_make_comp_char() to read the 
 * bounding box data from the font.
 * Sets Pmin and Pmax to the bottom left and top right corners
 * of the bounding box after transformation into device space.
 * The units of Pmin and Pmax are sub-pixels.
 * Updates *ppointer to point to the byte following the
 * bounding box data.
 */
ufix8 *sp_read_bbox(SPD_PROTO_DECL2
	ufix8 *pointer,	/* Pointer to next byte in char data */
	point_t *pPmin,	/* Lower left corner of bounding box */
	point_t *pPmax,	/* Upper right corner of bounding box */
	boolean set_flag)	/* flag to indicate whether global oru bbox should be saved */
{
	ufix8 format1;
	ufix8 format = 0;
	fix15 i;
	point_t P;

	UNUSED(set_flag);
	sp_globals.x_int = 0;
	sp_globals.y_int = sp_globals.Y_int_org;
	sp_globals.x_orus = sp_globals.y_orus = 0;
	format1 = NEXT_BYTE(pointer);
	pointer = sp_get_args(SPD_GARGS pointer, format1, pPmin);
#if INCL_SQUEEZING || INCL_ISW
	if (set_flag)
	{
		sp_globals.bbox_xmin_orus = sp_globals.x_orus;
		sp_globals.bbox_ymin_orus = sp_globals.y_orus;
	}
#endif
	*pPmax = *pPmin;
	for (i = 1; i < 4; i++)
	{
		switch (i)
		{
		case 1:
			if (format1 & BIT6)			/* Xmax requires X int zone 1? */
				sp_globals.x_int++;
			format = (format1 >> 4) | 0x0c;
			break;

		case 2:
			if (format1 & BIT7)			/* Ymax requires Y int zone 1? */
				sp_globals.y_int++;
			format = NEXT_BYTE(pointer);
			break;

		case 3:
			sp_globals.x_int = 0;
			format >>= 4;
			break;

		default:
			break;
		}

		pointer = sp_get_args(SPD_GARGS pointer, format, &P);
#if INCL_SQUEEZING || INCL_ISW
		if (set_flag && (i == 2))
		{
			sp_globals.bbox_xmax_orus = sp_globals.x_orus;
			sp_globals.bbox_ymax_orus = sp_globals.y_orus;
		}
#endif
		if ((i == 2) || (!sp_globals.normal))
		{
			if (P.x < pPmin->x)
				pPmin->x = P.x;
			if (P.y < pPmin->y)
				pPmin->y = P.y;
			if (P.x > pPmax->x)
				pPmax->x = P.x;
			if (P.y > pPmax->y)
				pPmax->y = P.y;
		}
	}

#if DEBUG
	printf("BBOX %6.1f(Xint 0), %6.1f(Yint 0), %6.1f(Xint %d), %6.1f(Yint %d)\n",
		   (double) pPmin->x / (double) sp_globals.onepix,
		   (double) pPmin->y / (double) sp_globals.onepix,
		   (double) pPmax->x / (double) sp_globals.onepix,
		   (format1 >> 6) & 0x01, (double) pPmax->y / (double) sp_globals.onepix, (format1 >> 7) & 0x01);

#endif
	return pointer;
}


/*
 * Called by sp_proc_outl_data() to subdivide Bezier curves into an
 * appropriate number of vectors, whenever curves are not enabled
 * for output to the currently selected output module.
 * sp_split_curve() calls itself recursively to the depth specified
 * at which point it calls line() to deliver each vector resulting
 * from the spliting process.
 */
static void sp_split_curve(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth)
{
	fix31 X0 = sp_globals.P0.x;
	fix31 Y0 = sp_globals.P0.y;
	fix31 X1 = x1;
	fix31 Y1 = y1;
	fix31 X2 = x2;
	fix31 Y2 = y2;
	fix31 X3 = x3;
	fix31 Y3 = y3;
	point_t Pmid, P3;
	point_t Pctrl1;
	point_t Pctrl2;

#if DEBUG
	printf("CRVE(%3.1f, %3.1f, %3.1f, %3.1f, %3.1f, %3.1f)\n",
		   (double) x1 / (double) sp_globals.onepix, (double) y1 / (double) sp_globals.onepix,
		   (double) x2 / (double) sp_globals.onepix, (double) y2 / (double) sp_globals.onepix,
		   (double) x3 / (double) sp_globals.onepix, (double) y3 / (double) sp_globals.onepix);
#endif

	P3.x = x3;
	P3.y = y3;
	Pmid.x = (X0 + (X1 + X2) * 3 + X3 + 4) >> 3;
	Pmid.y = (Y0 + (Y1 + Y2) * 3 + Y3 + 4) >> 3;
	if (--depth <= 0)
	{
		fn_line(Pmid);
		sp_globals.P0 = Pmid;
		fn_line(P3);
		sp_globals.P0 = P3;
	} else
	{
		Pctrl1.x = (X0 + X1 + 1) >> 1;
		Pctrl1.y = (Y0 + Y1 + 1) >> 1;
		Pctrl2.x = (X0 + (X1 << 1) + X2 + 2) >> 2;
		Pctrl2.y = (Y0 + (Y1 << 1) + Y2 + 2) >> 2;
		sp_split_curve(SPD_GARGS Pctrl1.x, Pctrl1.y, Pctrl2.x, Pctrl2.y, Pmid.x, Pmid.y, depth);
		Pctrl1.x = (X1 + (X2 << 1) + X3 + 2) >> 2;
		Pctrl1.y = (Y1 + (Y2 << 1) + Y3 + 2) >> 2;
		Pctrl2.x = (X2 + X3 + 1) >> 1;
		Pctrl2.y = (Y2 + Y3 + 1) >> 1;
		sp_split_curve(SPD_GARGS Pctrl1.x, Pctrl1.y, Pctrl2.x, Pctrl2.y, P3.x, P3.y, depth);
	}
}


/*
 * Called by sp_make_simp_char() and sp_make_comp_char() to read the 
 * outline data from the font.
 * The outline data is parsed, transformed into device coordinates
 * and passed to an output module for further processing.
 * Note that pointer is not updated to facilitate repeated
 * processing of the outline data when banding mode is in effect.
 */
void sp_proc_outl_data(SPD_PROTO_DECL2 ufix8 *pointer)	/* Pointer to next byte in char data */
{
	ufix8 format1, format2;
	point_t P0, P1, P2, P3;
	fix15 depth;
	fix15 curve_count;

	sp_globals.x_int = 0;
	sp_globals.y_int = sp_globals.Y_int_org;
#if INCL_PLAID_OUT						/* Plaid data monitoring included? */
	record_xint((fix15) sp_globals.x_int);	/* Record xint data */
	record_yint((fix15) (sp_globals.y_int - sp_globals.Y_int_org));	/* Record yint data */
#endif

	sp_globals.x_orus = sp_globals.y_orus = 0;
	curve_count = 0;
	for (;;)
	{
		format1 = NEXT_BYTE(pointer);
		switch (format1 >> 4)
		{
		case 0:						/* LINE */
			pointer = sp_get_args(SPD_GARGS pointer, format1, &P1);
#if DEBUG
			printf("LINE %6.1f, %6.1f\n",
				   (double) P1.x / (double) sp_globals.onepix, (double) P1.y / (double) sp_globals.onepix);
#endif
			fn_line(P1);
			sp_globals.P0 = P1;
			continue;

		case 1:						/* Short XINT */
			sp_globals.x_int = format1 & 0x0f;
#if DEBUG
			printf("XINT %d\n", sp_globals.x_int);
#endif
#if INCL_PLAID_OUT						/* Plaid data monitoring included? */
			record_xint((fix15) sp_globals.x_int);	/* Record xint data */
#endif
			continue;

		case 2:						/* Short YINT */
			sp_globals.y_int = sp_globals.Y_int_org + (format1 & 0x0f);
#if DEBUG
			printf("YINT %d\n", sp_globals.y_int - sp_globals.Y_int_org);
#endif
#if INCL_PLAID_OUT						/* Plaid data monitoring included? */
			record_yint((fix15) (sp_globals.y_int - sp_globals.Y_int_org));	/* Record yint data */
#endif
			continue;

		case 3:						/* Miscellaneous */
			switch (format1 & 0x0f)
			{
			case 0:					/* END */
				if (curve_count)
				{
					fn_end_contour();
				}
				return;

			case 1:					/* Long XINT */
				sp_globals.x_int = NEXT_BYTE(pointer);
#if DEBUG
				printf("XINT %d\n", sp_globals.x_int);
#endif
#if INCL_PLAID_OUT						/* Plaid data monitoring included? */
				record_xint((fix15) sp_globals.x_int);	/* Record xint data */
#endif
				continue;

			case 2:					/* Long YINT */
				sp_globals.y_int = sp_globals.Y_int_org + NEXT_BYTE(pointer);
#if DEBUG
				printf("YINT %d\n", sp_globals.y_int - sp_globals.Y_int_org);
#endif
#if INCL_PLAID_OUT						/* Plaid data monitoring included? */
				record_yint((fix15) (sp_globals.y_int - sp_globals.Y_int_org));	/* Record yint data */
#endif
				continue;

			default:					/* Not used */
				continue;
			}

		case 4:						/* MOVE Inside */
		case 5:						/* MOVE Outside */
			if (curve_count++)
			{
				fn_end_contour();
			}

			pointer = sp_get_args(SPD_GARGS pointer, format1, &P0);
			sp_globals.P0 = P0;
#if DEBUG
			printf("MOVE %6.1f, %6.1f\n",
				   (double) sp_globals.P0.x / (double) sp_globals.onepix,
				   (double) sp_globals.P0.y / (double) sp_globals.onepix);
#endif
			fn_begin_contour(sp_globals.P0, (boolean) (format1 & BIT4));
			continue;

		case 6:						/* Undefined */
#if DEBUG
			printf("*** Undefined instruction (Hex %4x)\n", format1);
#endif
			continue;

		case 7:						/* Undefined */
#if DEBUG
			printf("*** Undefined instruction (Hex %4x)\n", format1);
#endif
			continue;

		default:						/* CRVE */
			format2 = NEXT_BYTE(pointer);
			pointer = sp_get_args(SPD_GARGS pointer, format1, &P1);
			pointer = sp_get_args(SPD_GARGS pointer, format2, &P2);
			pointer = sp_get_args(SPD_GARGS pointer, (ufix8) (format2 >> 4), &P3);
			depth = (format1 >> 4) & 0x07;
#if DEBUG
			printf("CRVE %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %6.1f, %d\n",
				   (double) P1.x / (double) sp_globals.onepix, (double) P1.y / (double) sp_globals.onepix,
				   (double) P2.x / (double) sp_globals.onepix, (double) P2.y / (double) sp_globals.onepix,
				   (double) P3.x / (double) sp_globals.onepix, (double) P3.y / (double) sp_globals.onepix, depth);
#endif
			depth += sp_globals.depth_adj;
			if (sp_globals.specs.flags & CURVES_OUT)
			{
				fn_curve(P1, P2, P3, depth);
				sp_globals.P0 = P3;
				continue;
			}
			if (depth <= 0)
			{
				fn_line(P3);
				sp_globals.P0 = P3;
				continue;
			}
			sp_split_curve(SPD_GARGS P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, depth);
			continue;
		}
	}
}
