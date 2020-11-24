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

/****************************** H T E S T . C ********************************
 *                                                                           *
 *              SPEEDO FONT HEADER TEST MODULE                               *
 *                                                                           *
 ****************************************************************************/


#include "linux/libcwrap.h"
#include "speedo.h"						/* General definition for make_bmap */
#include <stdio.h>
#include <stdlib.h>

#define FONT_BUFFER_SIZE  1000

/***** PRIVATE FONT HEADER OFFSET CONSTANTS  *****/
#define  FH_ORUMX    0u     /* U   Max ORU value  2 bytes                   */
#define  FH_PIXMX    2u     /* U   Max Pixel value  2 bytes                 */
#define  FH_CUSNR    4u     /* U   Customer Number  2 bytes                 */
#define  FH_OFFCD    6u     /* E   Offset to Char Directory  3 bytes        */
#define  FH_OFCNS    9u     /* E   Offset to Constraint Data  3 bytes       */
#define  FH_OFFTK   12u     /* E   Offset to Track Kerning  3 bytes         */
#define  FH_OFFPK   15u     /* E   Offset to Pair Kerning  3 bytes          */
#define  FH_OCHRD   18u     /* E   Offset to Character Data  3 bytes        */
#define  FH_NBYTE   21u     /* E   Number of Bytes in File  3 bytes         */

static const char *pathname;				/* Name of font file to be output */

static ufix8 font_buffer[FONT_BUFFER_SIZE];	/* Font buffer */

static FILE *fdescr;					/* Speedo outline file descriptor */

#if STATIC_ALLOC
SPEEDO_GLOBALS sp_globals;
#elif DYNAMIC_ALLOC
SPEEDO_GLOBALS *sp_global_ptr;
#endif


/*
 * Reads 1-byte field from font buffer 
 */
static ufix8 read_1b(ufix8 *pointer)
{
	return *pointer;
}


/*
 * Reads 2-byte field from font buffer 
 */
static fix15 read_2b(ufix8 *pointer)
{
	fix31 temp;

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


ufix16 sp_get_cust_no(SPD_PROTO_DECL2 const buff_t *font_buff)
{
	ufix8 *hdr2_org;
	ufix16 private_off;

	SPD_GUNUSED
	private_off = read_2b(font_buff->org + FH_HEDSZ);
	if ((private_off + FH_CUSNR) > font_buff->no_bytes)
	{
		return 0;
	}

	hdr2_org = font_buff->org + private_off;

	return read_2b(hdr2_org + FH_CUSNR);
}


/*
 * Reads a 3-byte encrypted integer from the byte string starting at
 * the specified point.
 * Returns the decrypted value read as a signed integer.
 */
fix31 sp_read_long(SPD_PROTO_DECL2 ufix8 *pointer)	/* Pointer to first byte of encrypted 3-byte integer */
{
	fix31 tmpfix31;

	tmpfix31 = (fix31) ((*pointer++) ^ sp_globals.key4) << 8;	/* Read middle byte */
	tmpfix31 += (fix31) (*pointer++) << 16;	/* Read most significant byte */
	tmpfix31 += (fix31) ((*pointer) ^ sp_globals.key6);	/* Read least significant byte */
	return tmpfix31;
}


int main(int argc, char **argv)
{
	long bytes_read;					/* Number of bytes read from font file */
	ufix8 tmpufix8;						/* Temporary workspace */
	ufix32 tmpufix32;					/* Temporary workspace */
	ufix16 private_off;
	buff_t font;
	const ufix8 *key;
	ufix16 orus_per_em;

#if !DYNAMIC_ALLOC && !STATIC_ALLOC
	SPEEDO_GLOBALS *sp_global_ptr;
#endif

#if !STATIC_ALLOC
	sp_global_ptr = calloc(1, sizeof(*sp_global_ptr));
#endif

	if (argc != 2)
	{
		fprintf(stderr, "Usage: spdinfo {fontfile}\n");
		return 1;
	}

	pathname = argv[1];

	/* Initialization */
	/* Load Speedo outline file */
	fdescr = fopen(pathname, "rb");
	if (fdescr == NULL)
	{
		fprintf(stderr, "Cannot open file %s\n", pathname);
		return 1;
	}

	bytes_read = fread(font_buffer, sizeof(ufix8), sizeof(font_buffer), fdescr);
	if (bytes_read <= 0)
	{
		fprintf(stderr, "Error on reading %s: %ld\n", pathname, bytes_read);
		fclose(fdescr);
		return 1;
	}

	font.org = font_buffer;
	font.no_bytes = bytes_read;

	sp_reset_key(SPD_GARG1);
	if ((key = sp_get_key(SPD_GARG2 &font)) != NULL)
		sp_set_key(SPD_GARG2 key);

	printf("Format Identifier: %.4s\n", font_buffer + FH_FMVER);

	tmpufix32 = read_4b(font_buffer + FH_FMVER + 4);
	printf("CR-LF-NULL-NULL data: %8.8lx\n", (unsigned long)tmpufix32);

	printf("Font Size: %ld\n", (long)(ufix32) read_4b(font_buffer + FH_FNTSZ));

	printf("Minimum Font Buffer Size: %ld\n", (long)(ufix32) read_4b(font_buffer + FH_FBFSZ));

	printf("Minimum Character Buffer Size: %d\n", (ufix16) read_2b(font_buffer + FH_CBFSZ));

	private_off = (ufix16) read_2b(font_buffer + FH_HEDSZ);
	printf("Header Size: %d\n", private_off);

	printf("Font ID: %4.4d\n", (ufix16) read_2b(font_buffer + FH_FNTID));

	printf("Font Version Number: %d\n", (ufix16) read_2b(font_buffer + FH_SFVNR));

	printf("Font Full Name: %.70s\n", font_buffer + FH_FNTNM);

	printf("Manufacturing Date: %10.10s\n", font_buffer + FH_MDATE);

	printf("Character Set Name: %s\n", font_buffer + FH_LAYNM);

	printf("Character Set ID: %.4s\n", font_buffer + FH_LAYNM + 66);

	printf("Copyright Notice: %.70s\n", font_buffer + FH_CPYRT);

	printf("Number of Char. Indexes in Char. Set: %d\n", (ufix16) read_2b(font_buffer + FH_NCHRL));

	printf("Total number of Char. Indexes in Font: %d\n", (ufix16) read_2b(font_buffer + FH_NCHRF));

	printf("Index of First Character: %d\n", (ufix16) read_2b(font_buffer + FH_FCHRF));

	printf("Number of Kerning Tracks: %d\n", (ufix16) read_2b(font_buffer + FH_NKTKS));

	printf("Number of Kerning Pairs: %d\n", (ufix16) read_2b(font_buffer + FH_NKPRS));

	printf("Font Flags: 0x%x\n", read_1b(font_buffer + FH_FLAGS));

	printf("Classification Flags: 0x%x\n", read_1b(font_buffer + FH_CLFGS));

	printf("Family Classification: %d\n", read_1b(font_buffer + FH_FAMCL));

	tmpufix8 = read_1b(font_buffer + FH_FRMCL);
	printf("Font Form Width: %d\n", tmpufix8 & 0x0f);
	printf("Font Form Weight: %d\n", tmpufix8 >> 4);

	printf("Short Font Name: %.16s\n", font_buffer + FH_SFNTN);

	printf("Short Face Name: %.16s\n", font_buffer + FH_SFACN);

	printf("Font Form: %.14s\n", font_buffer + FH_FNTFM);

	printf("Italic Angle: %.2f\n", ((double) (fix15) read_2b(font_buffer + FH_ITANG) / 256.0));

	orus_per_em = read_2b(font_buffer + FH_ORUPM);
	printf("ORUs per Em: %d\n", orus_per_em);

	printf("Width of Word Space: %d\n", (ufix16) read_2b(font_buffer + FH_WDWTH));

	printf("Width of Em Space: %d\n", (ufix16) read_2b(font_buffer + FH_EMWTH));

	printf("Width of En Space: %d\n", (ufix16) read_2b(font_buffer + FH_ENWTH));

	printf("Width of Thin Space: %d\n", (ufix16) read_2b(font_buffer + FH_TNWTH));

	printf("Width of Figure Space: %d\n", (ufix16) read_2b(font_buffer + FH_FGWTH));

	printf("Min X coordinate in font: %d\n", read_2b(font_buffer + FH_FXMIN));

	printf("Min Y coordinate in font: %d\n", read_2b(font_buffer + FH_FYMIN));

	printf("Max X coordinate in font: %d\n", read_2b(font_buffer + FH_FXMAX));

	printf("Max Y coordinate in font: %d\n", read_2b(font_buffer + FH_FYMAX));

	printf("Underline Position: %d\n", read_2b(font_buffer + FH_ULPOS));

	printf("Underline Thickness: %d\n", read_2b(font_buffer + FH_ULTHK));

	printf("Small Caps Y position: %d\n", read_2b(font_buffer + FH_SMCTR));
	printf("Small Caps X scale: %.2f\n", ((double) read_2b(font_buffer + FH_SMCTR + 2) / 4096.0));
	printf("Small Caps Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_SMCTR + 4) / 4096.0));

	printf("Display Superiors Y position: %d\n", read_2b(font_buffer + FH_DPSTR));
	printf("Display Superiors X scale: %.2f\n", ((double) read_2b(font_buffer + FH_DPSTR + 2) / 4096.0));
	printf("Display Superiors Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_DPSTR + 4) / 4096.0));

	printf("Footnote Superiors Y position: %d\n", read_2b(font_buffer + FH_FNSTR));
	printf("Footnote Superiors X scale: %.2f\n", ((double) read_2b(font_buffer + FH_FNSTR + 2) / 4096.0));
	printf("Footnote Superiors Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_FNSTR + 4) / 4096.0));

	printf("Alpha Superiors Y position: %d\n", read_2b(font_buffer + FH_ALSTR));
	printf("Alpha Superiors X scale: %.2f\n", ((double) read_2b(font_buffer + FH_ALSTR + 2) / 4096.0));
	printf("Alpha Superiors Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_ALSTR + 4) / 4096.0));

	printf("Chemical Inferiors Y position: %d\n", read_2b(font_buffer + FH_CMITR));
	printf("Chemical Inferiors X scale: %.2f\n", ((double) read_2b(font_buffer + FH_CMITR + 2) / 4096.0));
	printf("Chemical Inferiors Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_CMITR + 4) / 4096.0));

	printf("Small Numerators Y position: %d\n", read_2b(font_buffer + FH_SNMTR));
	printf("Small Numerators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_SNMTR + 2) / 4096.0));
	printf("Small Numerators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_SNMTR + 4) / 4096.0));

	printf("Small Denominators Y position: %d\n", read_2b(font_buffer + FH_SDNTR));
	printf("Small Denominators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_SDNTR + 2) / 4096.0));
	printf("Small Denominators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_SDNTR + 4) / 4096.0));

	printf("Medium Numerators Y position: %d\n", read_2b(font_buffer + FH_MNMTR));
	printf("Medium Numerators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_MNMTR + 2) / 4096.0));
	printf("Medium Numerators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_MNMTR + 4) / 4096.0));

	printf("Medium Denominators Y position: %d\n", read_2b(font_buffer + FH_MDNTR));
	printf("Medium Denominators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_MDNTR + 2) / 4096.0));
	printf("Medium Denominators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_MDNTR + 4) / 4096.0));

	printf("Large Numerators Y position: %d\n", read_2b(font_buffer + FH_LNMTR));
	printf("Large Numerators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_LNMTR + 2) / 4096.0));
	printf("Large Numerators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_LNMTR + 4) / 4096.0));

	printf("Large Denominators Y position: %d\n", read_2b(font_buffer + FH_LDNTR));
	printf("Large Denominators X scale: %.2f\n", ((double) read_2b(font_buffer + FH_LDNTR + 2) / 4096.0));
	printf("Large Denominators Y scale: %.2f\n", ((double) read_2b(font_buffer + FH_LDNTR + 4) / 4096.0));

	if (private_off >= EXP_FH_METRES)
	{
		printf("Metric resolution: %d\n", read_2b(font_buffer + EXP_FH_METRES));
	} else
	{
		printf("Metric resolution: %d (default)\n", orus_per_em);
	}

	printf("\n** private header **\n\n");
	
	if (private_off + FH_CUSNR > (unsigned long)bytes_read)
	{
		printf("no private header\n");
	} else
	{
		ufix8 *hdr2_org;

		hdr2_org = font_buffer + private_off;
		printf("Max ORU value: %u\n", read_2b(hdr2_org + FH_ORUMX));
		printf("Max Pixel value: %u\n", read_2b(hdr2_org + FH_PIXMX));
		printf("Customer Number: %u\n", read_2b(hdr2_org + FH_CUSNR));
		printf("Offset to Char Directory: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_OFFCD));
		printf("Offset to Constraint Data: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_OFCNS));
		printf("Offset to Track Kerning: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_OFFTK));
		printf("Offset to Pair Kerning: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_OFFPK));
		printf("Offset to Character Data: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_OCHRD));
		printf("Number of Bytes in File: %lu\n", (unsigned long)sp_read_long(SPD_GARG2 hdr2_org + FH_NBYTE));
	}

	fclose(fdescr);

	return 0;
}
