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

/***************************** D O - C H A R . C *****************************
 *                                                                           *
 * This is the top level module for processing one simple or composite       *
 * character.
 *                                                                           *
 ****************************************************************************/

#include "linux/libcwrap.h"
#include "spdo_prv.h"					/* General definitions for Speedo    */

#if INCL_LCD
/*
 * Called by sp_get_char_id(), sp_get_char_width(), sp_make_char() and
 * sp_make_comp_char() to get a pointer to the start of the character data
 * for the specified character index.
 * Version for configuration supporting dynamic character data loading.
 * Calls sp_load_char_data() to load character data if not already loaded
 * as part of the original font buffer.
 * Returns NULL if character data not available
 */
static ufix8 *sp_get_char_org(SPD_PROTO_DECL2 ufix16 char_index,	/* Index of character to be accessed */
											   boolean top_level)	/* Not a compound character element */
{
	buff_t char_data;					/* Buffer descriptor requested */
	ufix8 *pointer;						/* Pointer into character directory */
	ufix8 format;						/* Character directory format byte */
	long char_offset;					/* Offset of char data from start of font file */
	long next_char_offset;				/* Offset of char data from start of font file */
	ufix16 no_bytes;					/* Number of bytes required for char data */

	if (top_level)						/* Not element of compound char? */
	{
		if (char_index < sp_globals.first_char_idx)	/* Before start of character set? */
			return NULL;
		char_index -= sp_globals.first_char_idx;
		if (char_index >= sp_globals.no_chars_avail)	/* Beyond end of character set? */
			return NULL;
		sp_globals.cb_offset = 0;		/* Reset char buffer offset */
	}

	pointer = sp_globals.pchar_dir;
	format = NEXT_BYTE(pointer);		/* Read character directory format byte */
	pointer += char_index << 1;			/* Point to indexed character entry */
	if (format)							/* 3-byte entries in char directory? */
	{
		pointer += char_index;			/* Adjust for 3-byte entries */
		char_offset = sp_read_long(SPD_GARGS pointer);	/* Read file offset to char data */
		next_char_offset = sp_read_long(SPD_GARGS pointer + 3);	/* Read offset to next char */
	} else
	{
		char_offset = 0xffffL & NEXT_WORD(pointer);	/* Read file offset to char data */
		next_char_offset = 0xffffL & NEXT_WORD(pointer);	/* Read offset to next char */
	}

	no_bytes = next_char_offset - char_offset;
	if (no_bytes == 0)					/* Character not in directory? */
		return NULL;

	if (next_char_offset <= sp_globals.font_buff_size)	/* Character data already in font buffer? */
		return sp_globals.font.org + char_offset;	/* Return pointer into font buffer */

	/* Request char data load */
	if (sp_load_char_data(SPD_GARGS char_offset, no_bytes, sp_globals.cb_offset, &char_data) == FALSE ||
		char_data.no_bytes < no_bytes)
		return NULL;

	if (top_level)						/* Not element of compound char? */
	{
		sp_globals.cb_offset = no_bytes;
	}

	return char_data.org;				/* Return pointer into character data buffer */
}

#else

/*
 * Called by sp_get_char_id(), sp_get_char_width(), sp_make_char() and
 * sp_make_comp_char() to get a pointer to the start of the character data
 * for the specified character index.
 * Version for configuration not supporting dynamic character data loading.
 * Returns NULL if character data not available
 */
static ufix8 *sp_get_char_org(SPD_PROTO_DECL2 ufix16 char_index,	/* Index of character to be accessed */
											   boolean top_level)	/* Not a compound character element */
{
	ufix8 *pointer;						/* Pointer into character directory */
	ufix8 format;						/* Character directory format byte */
	long char_offset;					/* Offset of char data from start of font file */
	long next_char_offset;				/* Offset of char data from start of font file */
	fix15 no_bytes;						/* Number of bytes required for char data */

	if (top_level)						/* Not element of compound char? */
	{
		if (char_index < sp_globals.first_char_idx)	/* Before start of character set? */
			return NULL;
		char_index -= sp_globals.first_char_idx;
		if (char_index >= sp_globals.no_chars_avail)	/* Beyond end of character set? */
			return NULL;
	}

	pointer = sp_globals.pchar_dir;
	format = NEXT_BYTE(pointer);		/* Read character directory format byte */
	pointer += char_index << 1;			/* Point to indexed character entry */
	if (format)							/* 3-byte entries in char directory? */
	{
		pointer += char_index;			/* Adjust for 3-byte entries */
		char_offset = sp_read_long(SPD_GARGS pointer);	/* Read file offset to char data */
		next_char_offset = sp_read_long(SPD_GARGS pointer + 3);	/* Read offset to next char */
	} else
	{
		char_offset = 0xffffL & NEXT_WORD(pointer);	/* Read file offset to char data */
		next_char_offset = 0xffffL & NEXT_WORD(pointer);	/* Read offset to next char */
	}

	no_bytes = next_char_offset - char_offset;
	if (no_bytes == 0)					/* Character not in directory? */
		return NULL;

	return sp_globals.font.org + char_offset;	/* Return pointer into font buffer */
}
#endif


/*
 * Returns character id for specified character index in currently
 * selected font.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 12 and returns 0 if character data not available.
 */
ufix16 sp_get_char_id(SPD_PROTO_DECL2 ufix16 char_index)	/* Index to character in char directory */
{
	ufix8 *pointer;						/* Pointer to character data */

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return 0;						/* Return zero character id */
	}

	pointer = sp_get_char_org(SPD_GARGS char_index, TRUE);	/* Get pointer to character data */
	if (pointer == NULL)				/* Character data not available? */
	{
		sp_report_error(SPD_GARGS 12);	/* Report character data not avail */
		return 0;						/* Return zero character id */
	}

	return 0xffff & NEXT_WORD(pointer);	/* Return character id */
}


#if INCL_METRICS
/*
 * Returns character set width for specified character index in currently
 * selected font in units of 1/65536 em.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 12 and returns 0 if character data not available.
 */
fix31 sp_get_char_width(SPD_PROTO_DECL2 ufix16 char_index)	/* Index to character in char directory */
{
	ufix8 *pointer;				/* Pointer to character data */
	fix31 set_width;					/* Set width of character */

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return 0;						/* Return zero character width */
	}

	pointer = sp_get_char_org(SPD_GARGS char_index, TRUE);	/* Get pointer to character data */
	if (pointer == NULL)				/* Character data not available? */
	{
		sp_report_error(SPD_GARGS 12);	/* Report character data not avail */
		return 0;						/* Return zero character width */
	}

	pointer += 2;						/* Skip over character id */
	set_width = (fix31) NEXT_WORD(pointer);	/* Read set width  and Convert units */
	set_width = ((set_width << 16) + (sp_globals.metric_resolution >> 1)) / sp_globals.metric_resolution;
	return set_width;					/* Return in 1/65536 em units */
}
#endif


#if INCL_METRICS
/*
 * Returns inter-character spacing adjustment in units of 1/256
 * points for the specified kerning track and point size.
 * If the specified point size is larger than the maximum point
 * size for the specified track, the adjustment for the maximum
 * point size is used.
 * If the specified point size is smaller than the minimum point
 * size for the specified track, the adjustment for the minimum
 * point size is used.
 * If the specified point size is between the minimum point size
 * and the maximum point size for the specified track, the 
 * adjustment is interpolated linearly between the minimum and
 * maximum adjustments.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 13 and returns 0 if track kerning data not in font.
 */
fix15 sp_get_track_kern(SPD_PROTO_DECL2 fix15 track,	/* Track required (0 - 3) */
							  fix15 point_size)	/* Point size (units of whole points) */
{
	ufix8 *pointer;						/* Pointer to character data */
	fix15 no_tracks;					/* Number of kerning tracks in font */
	ufix8 format;						/* Track kerning format byte */
	fix15 i;							/* Track counter */
	fix15 min_pt_size = 0;				/* Minimum point size for track */
	fix15 max_pt_size = 0;				/* Maximum point size for track */
	fix15 min_adj = 0;					/* Adjustment for min point size */
	fix15 max_adj = 0;					/* Adjustment for max point size */
	fix31 delta_pt_size;				/* Max point size - min point size */
	fix31 delta_adj;					/* Min adjustment - max adjustment */
	fix15 adj = 0;						/* Interpolated adjustment */

	if (track == 0)						/* Track zero selected? */
	{
		return adj;						/* Return zero track kerning adjustment */
	}

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return adj;						/* Return zero track kerning adjustment */
	}

	no_tracks = sp_globals.kern.no_tracks;	/* Number of kerning tracks */
	if (track > no_tracks)				/* Required track not available? */
	{
		sp_report_error(SPD_GARGS 13);	/* Report track kerning data not avail */
		return adj;						/* Return zero track kerning adjustment */
	}

	pointer = sp_globals.kern.tkorg;	/* Point to start of track kern data */
	for (i = 0; i < track; i++)			/* Read until track required is read */
	{
		format = NEXT_BYTE(pointer);	/* Read track kerning format byte */
		min_pt_size = (format & BIT0) ? NEXT_WORD(pointer) : (fix15) NEXT_BYTE(pointer);
		min_adj = (format & BIT1) ? NEXT_WORD(pointer) : (fix15) NEXT_BYTE(pointer);
		max_pt_size = (format & BIT2) ? NEXT_WORD(pointer) : (fix15) NEXT_BYTE(pointer);
		max_adj = (format & BIT3) ? NEXT_WORD(pointer) : (fix15) NEXT_BYTE(pointer);
	}

	if (point_size <= min_pt_size)		/* Smaller than minimum point size? */
	{
		return min_adj;					/* Return minimum adjustment (1/256 points) */
	}

	if (point_size >= max_pt_size)		/* Larger than maximum point size? */
	{
		return max_adj;					/* Return maximum adjustment (1/256 points) */
	}

	delta_pt_size = max_pt_size - min_pt_size;
	delta_adj = min_adj - max_adj;
	adj = (fix15) (min_adj - (((fix31) (point_size - min_pt_size) * delta_adj + (delta_pt_size >> 1)) / delta_pt_size));
	return adj;							/* Return interpolated adjustment (1/256 points) */
}
#endif


#if INCL_METRICS
/*
 * Returns inter-character spacing adjustment in units of 1/65536 em
 * for the specified pair of characters.
 * Reports Error 10 and returns 0 if no font selected.
 * Reports Error 14 and returns 0 if pair kerning data not in font.
 */
fix31 sp_get_pair_kern(SPD_PROTO_DECL2 ufix16 char_index1,	/* Index to first character in char directory */
							 ufix16 char_index2)	/* Index to second character in char directory */
{
	ufix8 *origin;						/* Pointer to first kerning pair record */
	ufix8 *pointer;						/* Pointer to character data */
	ufix16 tmpufix16;					/* Temporary workspace */
	fix15 no_pairs;						/* Number of kerning pairs in font */
	ufix8 format;						/* Track kerning format byte */
	boolean long_id;					/* TRUE if 2-byte character ids */
	fix15 rec_size;						/* Number of bytes in kern pair record */
	fix15 n;							/* Number of remaining kern pairs */
	fix15 nn;							/* Number of kern pairs in first partition */
	fix15 base;							/* Index to first record in rem kern pairs */
	fix15 i;							/* Index to kern pair being tested */
	fix31 adj = 0;						/* Returned value of adjustment */
	fix15 adj_base = 0;					/* Adjustment base for relative adjustments */

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return adj;						/* Return zero pair kerning adjustment */
	}

	no_pairs = sp_globals.kern.no_pairs;	/* Number of kerning pairs */
	if (no_pairs == 0)					/* Pair kerning data not available? */
	{
		sp_report_error(SPD_GARGS 14);	/* Report pair kerning data not avail */
		return adj;						/* Return zero pair kerning adjustment */
	}

	pointer = sp_globals.kern.pkorg;	/* Point to start of pair kern data */
	format = NEXT_BYTE(pointer);		/* Read pair kerning format byte */
	if (!(format & BIT0))				/* One-byte adjustment values? */
		adj_base = NEXT_WORD(pointer);	/* Read base adjustment */
	origin = pointer;					/* First byte of kerning pair data */
	rec_size = format + 3;				/* Compute kerning pair record size */
	long_id = format & BIT1;			/* Set flag for 2-byte char index */

	n = no_pairs;						/* Consider all kerning pairs */
	base = 0;							/* Set base at first kern pair record */
	while (n != 0)						/* While 1 or more kern pairs remain ... */
	{
		nn = n >> 1;					/* Size of first partition */
		i = base + nn;					/* Index to record to be tested */
		pointer = origin + (i * rec_size);
		tmpufix16 = NEXT_CHNDX(pointer, long_id);
		if (char_index1 < tmpufix16)
		{
			n = nn;						/* Number remaining in first partition */
			continue;
		}
		if (char_index1 > tmpufix16)
		{
			n -= nn + 1;				/* Number remaining in second partition */
			base = i + 1;				/* Base index for second partition */
			continue;
		}
		tmpufix16 = NEXT_CHNDX(pointer, long_id);
		if (char_index2 < tmpufix16)
		{
			n = nn;						/* Number remaining in first partition */
			continue;
		}
		if (char_index2 > tmpufix16)
		{
			n -= nn + 1;				/* Number remaining in second partition */
			base = i + 1;				/* Base index for second partition */
			continue;
		}
		adj = (format & BIT0) ? (fix31) NEXT_WORD(pointer) : (fix31) (adj_base + (fix15) NEXT_BYTE(pointer));
		adj = ((adj << 16) + (sp_globals.orus_per_em >> 1)) / sp_globals.orus_per_em;	/* Convert units */
		n = 0;							/* No more to consider */
	}
	return adj;							/* Return pair kerning adjustment */
}
#endif


#if INCL_METRICS
/*
 *	returns true if character exists, false if it doesn't
 *	provides transformed character bounding box in 1/65536 pixels
 *	in the provided bbox_t structure.  Bounding box may be
 *	conservative in the event that the transformation is not
 *	normal or the character is compound.
 */
boolean sp_get_char_bbox(SPD_PROTO_DECL2 ufix16 char_index, bbox_t *bbox, boolean no_adj)
{
	ufix8 *pointer;
	fix15 tmp;
	fix15 format;
	ufix16 pix_adj;
	point_t Pmin, Pmax;

#if REENTRANT_ALLOC
	plaid_t plaid;

	sp_globals.plaid = &plaid;
#endif

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return FALSE;					/* Error return */
	}

	sp_init_tcb(SPD_GARG);				/* Initialize transformation control block */

	pointer = sp_get_char_org(SPD_GARGS char_index, TRUE);	/* Point to start of character data */
	if (pointer == NULL)				/* Character data not available? */
	{
		sp_report_error(SPD_GARGS 12);	/* Report character data not avail */
		return FALSE;					/* Error return */
	}

	pointer += 2;						/* Skip over character id */
	tmp = NEXT_WORD(pointer);			/* Read set width */

	format = NEXT_BYTE(pointer);
	if (format & BIT1)					/* Optional data in header? */
	{
		tmp = (ufix8) NEXT_BYTE(pointer);	/* Read size of optional data */
		pointer += tmp;					/* Skip optional data */
	}

	pointer = sp_plaid_tcb(SPD_GARGS pointer, format);	/* Process plaid data */
	pointer = sp_read_bbox(SPD_GARGS pointer, &Pmin, &Pmax, FALSE);	/* Read bounding box */

	if ((format & BIT0) && !no_adj)
	{
		pix_adj = sp_globals.onepix << 1;	/* Allow 2 pixel expansion ... */
		Pmin.x -= pix_adj;					/* ... of components of ... */
		Pmin.y -= pix_adj;					/* ... compound ... */
		Pmax.x += pix_adj;					/* ... character ... */
		Pmax.y += pix_adj;					/* ... bounding box. */
	}

	bbox->xmin = (fix31) Pmin.x << sp_globals.poshift;
	bbox->xmax = (fix31) Pmax.x << sp_globals.poshift;
	bbox->ymin = (fix31) Pmin.y << sp_globals.poshift;
	bbox->ymax = (fix31) Pmax.y << sp_globals.poshift;

	return TRUE;
}

#endif


#if INCL_ISW || INCL_SQUEEZING
static void sp_preview_bounding_box(SPD_PROTO_DECL2 ufix8 *pointer, ufix8 format)	/* Character format byte */
{
	point_t Pmin, Pmax;						/* Transformed corners of bounding box */

	sp_globals.no_X_orus = (format & BIT2) ? (fix15) NEXT_BYTE(pointer) : 0;
	sp_globals.no_Y_orus = (format & BIT3) ? (fix15) NEXT_BYTE(pointer) : 0;
	pointer = sp_read_oru_table(SPD_GARGS pointer);

	/* Skip over control zone table */
	pointer = sp_skip_control_zone(SPD_GARGS pointer, format);

	/* Skip over interpolation table */
	pointer = sp_skip_interpolation_table(SPD_GARGS pointer, format);
	/* sp_get_args has a pathological need for this value to be set */
	sp_globals.Y_edge_org = sp_globals.no_X_orus;
	pointer = sp_read_bbox(SPD_GARGS pointer, &Pmin, &Pmax, TRUE);	/* Read bounding box */

}
#endif


#if INCL_ISW
static boolean sp_reset_xmax(SPD_PROTO_DECL2 fix31 xmax)
{
	fix15 xmin;							/* Minimum X ORU value in font */
	fix15 ymin;							/* Minimum Y ORU value in font */
	fix15 ymax;							/* Maximum Y ORU value in font */

	sp_globals.isw_modified_constants = TRUE;
	xmin = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FXMIN);
	ymin = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FYMIN);
	ymax = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FYMAX);

	if (!sp_setup_consts(SPD_GARGS xmin, xmax, ymin, ymax))
	{
		sp_report_error(SPD_GARGS 3);	/* Requested specs out of range */
		return FALSE;
	}
	sp_globals.constr.data_valid = FALSE;
	/* recompute setwidth */
	sp_globals.Psw.x = (fix15) ((fix31) (((fix31) sp_globals.imported_width * (sp_globals.specs.xxmult >> 16) +
										  (((fix31) sp_globals.imported_width *
											(sp_globals.specs.xxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
								sp_globals.metric_resolution);
	sp_globals.Psw.y =
		(fix15) ((fix31)
				 (((fix31) sp_globals.imported_width * (sp_globals.specs.yxmult >> 16) +
				   (((fix31) sp_globals.imported_width *
					 (sp_globals.specs.yxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
				 sp_globals.metric_resolution);

	return TRUE;
}
#endif


/*
 * Called by sp_make_char() to output a simple (non-compound) character.
 * Returns TRUE on completion.
 */
static boolean sp_make_simp_char(SPD_PROTO_DECL2 ufix8 *pointer, ufix8 format)
{
	point_t Pmin, Pmax;								/* Transformed corners of bounding box */
#if INCL_SQUEEZING || INCL_ISW
	ufix8 *save_pointer;
#endif
#if INCL_ISW
	fix31 char_width;
	fix31 isw_scale;
#endif

#if INCL_SQUEEZING
	sp_globals.squeezing_compound = FALSE;
	if ((sp_globals.specs.flags & SQUEEZE_LEFT) ||
		(sp_globals.specs.flags & SQUEEZE_RIGHT) ||
		(sp_globals.specs.flags & SQUEEZE_TOP) ||
		(sp_globals.specs.flags & SQUEEZE_BOTTOM))
	{
		/* get the bounding box data before processing the character */
		save_pointer = pointer;
		sp_preview_bounding_box(SPD_GARGS pointer, format);
		pointer = save_pointer;
	}
#endif
#if INCL_ISW
	if (sp_globals.import_setwidth_act)
	{
		save_pointer = pointer;
		sp_preview_bounding_box(SPD_GARGS pointer, format);
		pointer = save_pointer;
		/* make sure I'm not going to get fixed point overflow */
		isw_scale = sp_compute_isw_scale(SPD_GARG);
		if (sp_globals.bbox_xmin_orus < 0)
			char_width = SQUEEZE_MULT((sp_globals.bbox_xmax_orus - sp_globals.bbox_xmin_orus), isw_scale);
		else
			char_width = SQUEEZE_MULT(sp_globals.bbox_xmax_orus, isw_scale);
		if (char_width >= sp_globals.isw_xmax)
			if (!sp_reset_xmax(SPD_GARGS char_width))
				return FALSE;
	}
#endif
	pointer = sp_plaid_tcb(SPD_GARGS pointer, format);	/* Process plaid data */
	pointer = sp_read_bbox(SPD_GARGS pointer, &Pmin, &Pmax, FALSE);	/* Read bounding box */
	if (fn_begin_char(sp_globals.Psw, Pmin, Pmax))	/* Signal start of character output */
	{
		do
		{
			sp_proc_outl_data(SPD_GARGS pointer);	/* Process outline data */
		} while (!fn_end_char());		/* Repeat if not done */
	}
	return TRUE;
}


/*
 * Called by sp_make_comp_char() to read a position argument from the
 * specified point in the font/char buffer.
 * Updates pointer to byte following position argument.
 * Returns value of position argument in outline resolution units
 */
static fix15 sp_get_posn_arg(SPD_PROTO_DECL2 ufix8 **ppointer, ufix8 format)	/* Format of DOCH arguments */
{
	switch (format & 0x03)
	{
	case 1:
		return NEXT_WORD(*ppointer);

	case 2:
		return (fix7) NEXT_BYTE(*ppointer);

	default:
		return 0;
	}
}


/*
 * Called by sp_make_comp_char() to read a scale argument from the
 * specified point in the font/char buffer.
 * Updates pointer to byte following scale argument.
 * Returns value of scale argument in scale units (normally 1/4096)
 */
static fix15 sp_get_scale_arg(SPD_PROTO_DECL2 ufix8 **ppointer, ufix8 format)	/* Format of DOCH arguments */
{
	if (format)
		return NEXT_WORD(*ppointer);
	return ONE_SCALE;
}


/*
 * Called by sp_make_char() to output a compound character.
 * Returns FALSE if data for any sub-character is not available.
 * Returns TRUE if output completed with no error.
 */
static boolean sp_make_comp_char(SPD_PROTO_DECL2 ufix8 *pointer)	/* Pointer to first byte of position argument */
{
	point_t Pmin, Pmax;					/* Transformed corners of bounding box */
	point_t Pssw;						/* Transformed escapement vector */
	ufix8 *pointer_sav;					/* Saved pointer to compound character data */
	ufix8 *sub_pointer;					/* Pointer to sub-character data */
	ufix8 format;						/* Format of DOCH instruction */
	ufix16 sub_char_index;				/* Index to sub-character in character directory */
	fix15 x_posn;						/* X position of sub-character (outline res units) */
	fix15 y_posn;						/* Y position of sub-character (outline res units) */
	fix15 x_scale;						/* X scale factor of sub-character (scale units) */
	fix15 y_scale;						/* Y scale factor of sub-character (scale units) */
	fix15 tmpfix15;						/* Temporary workspace */
	fix15 x_orus;						/* Set width in outline resolution units */
	fix15 pix_adj;						/* Pixel adjustment to compound char bounding box */

#if INCL_SQUEEZING
	fix31 x_factor, x_offset, top_scale, bottom_scale;
	boolean squeezed_x, squeezed_y;
#endif
#if INCL_SQUEEZING || INCL_ISW
	fix15 x_offset_pix;
#endif
#if INCL_ISW
	fix31 char_width;
	fix31 isw_scale;
#endif

#if INCL_SQUEEZING
	sp_globals.squeezing_compound = TRUE;
#endif
	pointer = sp_read_bbox(SPD_GARGS pointer, &Pmin, &Pmax, TRUE);	/* Read bounding box data */
	pix_adj = sp_globals.onepix << 1;	/* Allow 2 pixel expansion ... */
	Pmin.x -= pix_adj;					/* ... of components of ... */
	Pmin.y -= pix_adj;					/* ... compound ... */
	Pmax.x += pix_adj;					/* ... character ... */
	Pmax.y += pix_adj;					/* ... bounding box. */

#if INCL_SQUEEZING
	/* scale the bounding box if necessary before calling begin_char */
	squeezed_x = sp_calculate_x_scale(SPD_GARGS &x_factor, &x_offset, 0);
	squeezed_y = sp_calculate_y_scale(SPD_GARGS &top_scale, &bottom_scale, 0, 0);

	if (squeezed_x)
	{									/* scale the x coordinates of the bbox */
		x_offset_pix = (fix15) (((x_offset >> 16) * sp_globals.tcb0.xppo) >> sp_globals.mpshift);
		if ((x_offset_pix > 0) && (x_offset_pix < sp_globals.onepix))
			x_offset_pix = sp_globals.onepix;
		Pmin.x = SQUEEZE_MULT(x_factor, Pmin.x) + x_offset_pix - pix_adj;
		Pmax.x = SQUEEZE_MULT(x_factor, Pmax.x) + x_offset_pix + pix_adj;
	}
	if (squeezed_y)
	{									/* scale the y coordinates of the bbox */
		if ((Pmin.y) < 0)
			Pmin.y = SQUEEZE_MULT(bottom_scale, Pmin.y) - pix_adj;
		else
			Pmin.y = SQUEEZE_MULT(top_scale, Pmin.y) - pix_adj;
		if ((Pmax.y) < 0)
			Pmax.y = SQUEEZE_MULT(bottom_scale, Pmax.y) + pix_adj;
		else
			Pmax.y = SQUEEZE_MULT(top_scale, Pmax.y) + pix_adj;
	}
#endif
#if INCL_ISW
	if (sp_globals.import_setwidth_act)
	{
		/* make sure I'm not going to get fixed point overflow */
		isw_scale = ((fix31) sp_globals.imported_width << 16) / (fix31) sp_globals.setwidth_orus;
		char_width = SQUEEZE_MULT((sp_globals.bbox_xmax_orus - sp_globals.bbox_xmin_orus), isw_scale);
		if (char_width >= sp_globals.isw_xmax)
			if (!sp_reset_xmax(SPD_GARGS char_width))
				return FALSE;
	}
#endif

	if (fn_begin_char(sp_globals.Psw, Pmin, Pmax))	/* Signal start of character data */
	{
		pointer_sav = pointer;
		do
		{
			pointer = pointer_sav;		/* Point to next DOCH or END instruction */
			while ((format = NEXT_BYTE(pointer)) != 0)	/* DOCH instruction? */
			{
				sp_init_tcb(SPD_GARG);			/* Initialize transformation control block */
				x_posn = sp_get_posn_arg(SPD_GARGS &pointer, format);
				y_posn = sp_get_posn_arg(SPD_GARGS &pointer, (ufix8) (format >> 2));
				x_scale = sp_get_scale_arg(SPD_GARGS &pointer, (ufix8) (format & BIT4));
				y_scale = sp_get_scale_arg(SPD_GARGS &pointer, (ufix8) (format & BIT5));
				sp_scale_tcb(SPD_GARGS &sp_globals.tcb, x_posn, y_posn, x_scale, y_scale);	/* Scale for sub-char */
				sub_char_index = (format & BIT6) ?	/* Read sub-char index */
					NEXT_WORD(pointer) : NEXT_BYTE(pointer);
				sub_pointer = sp_get_char_org(SPD_GARGS sub_char_index, FALSE);	/* Point to start of sub-char */
				if (sub_pointer == NULL)	/* Character data not available? */
				{
					return FALSE;		/* Abort character output */
				}
				sub_pointer += 2;		/* Skip over character id */
				x_orus = NEXT_WORD(sub_pointer);	/* Read set_width of sub-character */

				Pssw.x = (fix15) ((fix31) (((fix31) x_orus * (sp_globals.specs.xxmult >> 16) +
											(((fix31) x_orus *
											  (sp_globals.specs.xxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
								  sp_globals.metric_resolution);
				Pssw.y =
					(fix15) ((fix31)
							 (((fix31) x_orus * (sp_globals.specs.yxmult >> 16) +
							   (((fix31) x_orus * (sp_globals.specs.yxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
							 sp_globals.metric_resolution);

				format = NEXT_BYTE(sub_pointer);	/* Read sub-character format */
				if (format & BIT1)		/* Optional data in header? */
				{
					tmpfix15 = (ufix8) NEXT_BYTE(sub_pointer);	/* Read size of optional data */
					sub_pointer += tmpfix15;	/* Skip optional data */
				}
				sub_pointer = sp_plaid_tcb(SPD_GARGS sub_pointer, format);	/* Process sub-character plaid data */
				sub_pointer = sp_read_bbox(SPD_GARGS sub_pointer, &Pmin, &Pmax, FALSE);	/* Read bounding box */
				fn_begin_sub_char(Pssw, Pmin, Pmax);	/* Signal start of sub-character data */
				sp_proc_outl_data(SPD_GARGS sub_pointer);	/* Process sub-character data */
				fn_end_sub_char();		/* Signal end of sub-character data */
			}
		} while (!fn_end_char());		/* Signal end of character; repeat if required */
	}
	return TRUE;
}


/*
 * Outputs specified character using the currently selected font and
 * scaling and output specifications.
 * Reports Error 10 and returns FALSE if no font specifications 
 * previously set.
 * Reports Error 12 and returns FALSE if character data not available.
 */
#if INCL_ISW
static boolean sp_do_make_char(SPD_PROTO_DECL2 ufix16 char_index)
#else
boolean sp_make_char(SPD_PROTO_DECL2 ufix16 char_index)
#endif
{
	ufix8 *pointer;				/* Pointer to character data */
	fix15 x_orus;
	fix15 tmpfix15;
	ufix8 format;

#if INCL_ISW
	sp_globals.isw_modified_constants = FALSE;
#endif

#if REENTRANT_ALLOC

	plaid_t plaid;

#if INCL_BLACK || INCL_SCREEN || INCL_2D
	intercepts_t intercepts;

	sp_globals.intercepts = &intercepts;
#endif

	sp_globals.plaid = &plaid;
#endif

	if (!sp_globals.specs_valid)		/* Font specs not defined? */
	{
		sp_report_error(SPD_GARGS 10);	/* Report font not specified */
		return FALSE;					/* Error return */
	}

#if INCL_MULTIDEV
#if INCL_OUTLINE
	if (sp_globals.specs.output_mode == MODE_OUTLINE && !sp_globals.outline_device_set)
	{
		sp_report_error(SPD_GARGS 2);	/* Transformation matrix out of range */
		return FALSE;
	} else
#endif
	if (!sp_globals.bitmap_device_set)
	{
		sp_report_error(SPD_GARGS 2);	/* Transformation matrix out of range */
		return FALSE;
	}
#endif

	sp_init_tcb(SPD_GARG);				/* Initialize transformation control block */

	pointer = sp_get_char_org(SPD_GARGS char_index, TRUE);	/* Point to start of character data */
	if (pointer == NULL)				/* Character data not available? */
	{
		sp_report_error(SPD_GARGS 12);	/* Character data not available */
		return FALSE;					/* Error return */
	}

	pointer += 2;						/* Skip over character id */
	x_orus = NEXT_WORD(pointer);		/* Read set width */
#if INCL_SQUEEZING || INCL_ISW
	sp_globals.setwidth_orus = x_orus;
#endif

#if INCL_ISW
	if (sp_globals.import_setwidth_act)
		x_orus = sp_globals.imported_width;
#endif
	sp_globals.Psw.x = (fix15) ((fix31)
								(((fix31) x_orus * (sp_globals.specs.xxmult >> 16) +
								  (((fix31) x_orus *
									(sp_globals.specs.xxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
								sp_globals.metric_resolution);

	sp_globals.Psw.y = (fix15) ((fix31) (((fix31) x_orus * (sp_globals.specs.yxmult >> 16) +
										  (((fix31) x_orus *
											(sp_globals.specs.yxmult & 0xffffL)) >> 16)) << sp_globals.pixshift) /
								sp_globals.metric_resolution);

	format = NEXT_BYTE(pointer);
	if (format & BIT1)					/* Optional data in header? */
	{
		tmpfix15 = (ufix8) NEXT_BYTE(pointer);	/* Read size of optional data */
		pointer += tmpfix15;			/* Skip optional data */
	}
	if (format & BIT0)
	{
		return sp_make_comp_char(SPD_GARGS pointer);	/* Output compound character */
	}
	return sp_make_simp_char(SPD_GARGS pointer, format);	/* Output simple character */
}


#if INCL_ISW
boolean sp_make_char_isw(SPD_PROTO_DECL2 ufix16 char_index, ufix32 imported_setwidth)
{
	fix15 xmin;							/* Minimum X ORU value in font */
	fix15 xmax;							/* Maximum X ORU value in font */
	fix15 ymin;							/* Minimum Y ORU value in font */
	fix15 ymax;							/* Maximum Y ORU value in font */
	boolean return_value;

	sp_globals.import_setwidth_act = TRUE;
	/* convert imported width to orus */
	sp_globals.imported_width = (sp_globals.metric_resolution * imported_setwidth) >> 16;
	return_value = sp_do_make_char(SPD_GARGS char_index);

	if (sp_globals.isw_modified_constants)
	{
		/* reset fixed point constants */
		xmin = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FXMIN);
		ymin = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FYMIN);
		xmax = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FXMAX);
		ymax = sp_read_word_u(SPD_GARGS sp_globals.font_org + FH_FYMAX);
		sp_globals.constr.data_valid = FALSE;
		if (!sp_setup_consts(SPD_GARGS xmin, xmax, ymin, ymax))
		{
			sp_report_error(SPD_GARGS 3);	/* Requested specs out of range */
			return FALSE;
		}
	}
	return return_value;
}


boolean sp_make_char(SPD_PROTO_DECL2 ufix16 char_index)	/* Index to character in char directory */
{
	sp_globals.import_setwidth_act = FALSE;
	return sp_do_make_char(SPD_GARGS char_index);
}
#endif
