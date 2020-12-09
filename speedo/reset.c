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



/******************************* R E S E T . C *******************************
 *                                                                           *
 * This module provides initialization functions.                            *
 *                                                                           *
 ****************************************************************************/

#include "linux/libcwrap.h"
#include "spdo_prv.h"					/* General definitions for Speedo     */

#define   DEBUG      0

#if DEBUG
#include <stdio.h>
#define SHOW(X) printf("X = %d\n", X)
#else
#define SHOW(X)
#endif



/*
 * Called by the host software to intialize the Speedo mechanism
 */
void sp_reset(SPD_PROTO_DECL1)
{
	sp_globals.specs_valid = FALSE;		/* Flag specs not valid */

	/* Reset decryption key */
	sp_reset_key(SPD_GARG);

#if INCL_RULES
	sp_globals.constr.font_id_valid = FALSE;
#endif

#if INCL_MULTIDEV
#if INCL_BLACK || INCL_SCREEN || INCL_2D
	sp_globals.bitmap_device_set = FALSE;
#endif
#if INCL_OUTLINE
	sp_globals.outline_device_set = FALSE;
#endif
#endif
}


/*
	returns customer number from font 
*/
ufix16 sp_get_cust_no(SPD_PROTO_DECL2 const buff_t *font_buff)
{
	ufix8 *hdr2_org;
	ufix16 private_off;

	private_off = sp_read_word_u(SPD_GARGS font_buff->org + FH_HEDSZ);
	if ((private_off + FH_CUSNR) > font_buff->no_bytes)
	{
		sp_report_error(SPD_GARGS 1);		/* Insufficient font data loaded */
		return FALSE;
	}

	hdr2_org = font_buff->org + private_off;

	return sp_read_word_u(SPD_GARGS hdr2_org + FH_CUSNR);
}
