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


#include "linux/libcwrap.h"
#include "speedo.h"

/***** DECRYPTION KEY CONSTANTS (PC Platform) *****/

#define CUS0  432                  /* Customer number */

/***** DECRYPTION KEY CONSTANTS (Sample) *****/

#define XCUS0    0                 /* Customer number */


#ifdef EXTRAFONTS
static ufix8 const skey[] = {
	SKEY0,
	SKEY1,
	SKEY2,
	SKEY3,
	SKEY4,
	SKEY5,
	SKEY6,
	SKEY7,
	SKEY8
};										/* Sample Font decryption key */

static ufix8 const rkey[] = {
	RKEY0,
	RKEY1,
	RKEY2,
	RKEY3,
	RKEY4,
	RKEY5,
	RKEY6,
	RKEY7,
	RKEY8
};										/* Retail Font decryption key */

#endif /* EXTRAFONTS */

static ufix8 const xkey[] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};										/* Sample Font decryption key */

static ufix8 const mkey[] = {
	  0,                /* Decryption key 0 */
	 72,                /* Decryption key 1 */
	123,                /* Decryption key 2 */
	  1,                /* Decryption key 3 */
	222,                /* Decryption key 4 */
	194,                /* Decryption key 5 */
	113,                /* Decryption key 6 */
	119,                /* Decryption key 7 */
	 52                 /* Decryption key 8 */
};										/* Font decryption key */

static ufix8 const nkey[] = {
	  0,                /* Decryption key 0 */
	205,                /* Decryption key 1 */
	 86,                /* Decryption key 2 */
	  0,                /* Decryption key 3 */
	255,                /* Decryption key 4 */
	 54,                /* Decryption key 5 */
	 56,                /* Decryption key 6 */
	218,                /* Decryption key 7 */
	  1                 /* Decryption key 8 */
};


/*
 * Dynamically sets font decryption key.
 */
void sp_set_key(SPD_PROTO_DECL2 const ufix8 *key)	/* Specified decryption key */
{
	sp_globals.key32 = (key[3] << 8) | key[2];
	sp_globals.key4 = key[4];
	sp_globals.key6 = key[6];
	sp_globals.key7 = key[7];
	sp_globals.key8 = key[8];
}


void sp_reset_key(SPD_PROTO_DECL1)
{
	sp_set_key(SPD_GARGS mkey);
}


const ufix8 *sp_get_key(SPD_PROTO_DECL2 const buff_t *font_buff)
{
	ufix16 cust_no;
	const ufix8 *key;

	cust_no = sp_get_cust_no(SPD_GARGS font_buff);
#ifdef EXTRAFONTS
	if (cust_no == SCUS0)
	{
		key = skey;
	} else if (cust_no == RCUS0)
	{
		key = rkey;
	} else
#endif

	if (cust_no == XCUS0)
	{
		key = xkey;
	} else if (cust_no == CUS0)
	{
		key = mkey;
	} else if (cust_no == 528)
	{
		key = nkey;
	} else
	{
		key = NULL;
	}
	return key;
}
