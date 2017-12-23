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

static const char *pathname;				/* Name of font file to be output */

static ufix8 font_buffer[FONT_BUFFER_SIZE];	/* Font buffer */

static FILE *fdescr;					/* Speedo outline file descriptor */



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


int main(int argc, char **argv)
{
	int bytes_read;						/* Number of bytes read from font file */
	ufix8 tmpufix8;						/* Temporary workspace */
	ufix32 tmpufix32;					/* Temporary workspace */

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
	if (bytes_read == 0)
	{
		fprintf(stderr, "Error on reading %s: %x\n", pathname, bytes_read);
		fclose(fdescr);
		return 1;
	}

	printf("Format Identifier: %.4s\n", font_buffer + FH_FMVER);

	tmpufix32 = (ufix32) read_4b(font_buffer + FH_FMVER + 4);
	printf("CR-LF-NULL-NULL data: %8.8lx\n", (unsigned long)tmpufix32);

	printf("Font Size: %ld\n", (long)(ufix32) read_4b(font_buffer + FH_FNTSZ));

	printf("Minimum Font Buffer Size: %ld\n", (long)(ufix32) read_4b(font_buffer + FH_FBFSZ));

	printf("Minimum Character Buffer Size: %d\n", (ufix16) read_2b(font_buffer + FH_CBFSZ));

	printf("Header Size: %d\n", (ufix16) read_2b(font_buffer + FH_HEDSZ));

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

	printf("Italic Angle: %.2f\n", ((real) read_2b(font_buffer + FH_ITANG) / 256.0));

	printf("ORUs per Em: %d\n", (ufix16) read_2b(font_buffer + FH_ORUPM));

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
	printf("Small Caps X scale: %.2f\n", ((real) read_2b(font_buffer + FH_SMCTR + 2) / 4096.0));
	printf("Small Caps Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_SMCTR + 4) / 4096.0));

	printf("Display Superiors Y position: %d\n", read_2b(font_buffer + FH_DPSTR));
	printf("Display Superiors X scale: %.2f\n", ((real) read_2b(font_buffer + FH_DPSTR + 2) / 4096.0));
	printf("Display Superiors Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_DPSTR + 4) / 4096.0));

	printf("Footnote Superiors Y position: %d\n", read_2b(font_buffer + FH_FNSTR));
	printf("Footnote Superiors X scale: %.2f\n", ((real) read_2b(font_buffer + FH_FNSTR + 2) / 4096.0));
	printf("Footnote Superiors Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_FNSTR + 4) / 4096.0));

	printf("Alpha Superiors Y position: %d\n", read_2b(font_buffer + FH_ALSTR));
	printf("Alpha Superiors X scale: %.2f\n", ((real) read_2b(font_buffer + FH_ALSTR + 2) / 4096.0));
	printf("Alpha Superiors Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_ALSTR + 4) / 4096.0));

	printf("Chemical Inferiors Y position: %d\n", read_2b(font_buffer + FH_CMITR));
	printf("Chemical Inferiors X scale: %.2f\n", ((real) read_2b(font_buffer + FH_CMITR + 2) / 4096.0));
	printf("Chemical Inferiors Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_CMITR + 4) / 4096.0));

	printf("Small Numerators Y position: %d\n", read_2b(font_buffer + FH_SNMTR));
	printf("Small Numerators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_SNMTR + 2) / 4096.0));
	printf("Small Numerators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_SNMTR + 4) / 4096.0));

	printf("Small Denominators Y position: %d\n", read_2b(font_buffer + FH_SDNTR));
	printf("Small Denominators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_SDNTR + 2) / 4096.0));
	printf("Small Denominators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_SDNTR + 4) / 4096.0));

	printf("Medium Numerators Y position: %d\n", read_2b(font_buffer + FH_MNMTR));
	printf("Medium Numerators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_MNMTR + 2) / 4096.0));
	printf("Medium Numerators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_MNMTR + 4) / 4096.0));

	printf("Medium Denominators Y position: %d\n", read_2b(font_buffer + FH_MDNTR));
	printf("Medium Denominators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_MDNTR + 2) / 4096.0));
	printf("Medium Denominators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_MDNTR + 4) / 4096.0));

	printf("Large Numerators Y position: %d\n", read_2b(font_buffer + FH_LNMTR));
	printf("Large Numerators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_LNMTR + 2) / 4096.0));
	printf("Large Numerators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_LNMTR + 4) / 4096.0));

	printf("Large Denominators Y position: %d\n", read_2b(font_buffer + FH_LDNTR));
	printf("Large Denominators X scale: %.2f\n", ((real) read_2b(font_buffer + FH_LDNTR + 2) / 4096.0));
	printf("Large Denominators Y scale: %.2f\n", ((real) read_2b(font_buffer + FH_LDNTR + 4) / 4096.0));

	fclose(fdescr);
	
	return 0;
}