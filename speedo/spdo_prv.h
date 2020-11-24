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



/***************************** S P D O _ P R V . H *******************************/
 
#include "speedo.h"  /* include public definitions */

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


/***** MACRO DEFINITIONS *****/

#define SQUEEZE_MULT(A,B) (((fix31)A * (fix31)B) / ((fix31)1 << 16))

#define NEXT_BYTE(A) (*(A)++)

#define NEXT_WORD(A) \
    ((fix15)(sp_globals.key32 ^ ((A) += 2, \
				 ((fix15)((A)[-1]) << 8) | (fix15)((A)[-2]) | ((A)[-1] & 0x80 ? ~0xFFFF : 0))))

#if INCL_EXT

#define NEXT_BYTES(A, B) \
    (((B = (ufix16)(*(A)++) ^ sp_globals.key7) >= 248) ? \
     ((ufix16)(B & 0x07) << 8) + ((*(A)++) ^ sp_globals.key8) + 248: \
     B)

#else

#define NEXT_BYTES(A, B) ((*(A)++) ^ sp_globals.key7)

#endif


#define NEXT_BYTE_U(A) (*(A)++) 

#define NEXT_WORD_U(A, B) \
    (fix15)(B = (*(A)++) << 8, (fix15)(*(A)++) + B)

#define NEXT_CHNDX(A, B) \
    ((B)? (ufix16)NEXT_WORD(A): (ufix16)NEXT_BYTE(A))

/* Multiply (fix15)X by (fix15)Y to produce (fix31)product */
#define MULT16(X, Y) ((fix31)X * (fix31)Y)

/* Multiply (fix15)X by (fix15)MULT, add (fix31)OFFSET, 
 * shift right SHIFT bits to produce (fix15)result */
#define TRANS(X, MULT, OFFSET, SHIFT) ((fix15)((((fix31)X * (fix31)MULT) + OFFSET) / ((fix31)1 << SHIFT)))

/******************************************************************************
 *
 *      the following block of definitions redefines every function
 *      reference to be prefixed with an "sp_".  In addition, if this 
 *      is a reentrant version, the parameter sp_globals will be added
 *      as the first parameter.
 *
 *****************************************************************************/

#define fn_init_out(specsarg) (*sp_globals.init_out)(SPD_GARG2 specsarg)  
#define fn_begin_char(Psw,Pmin,Pmax) (*sp_globals.begin_char)(SPD_GARG2 Psw.x, Psw.y, Pmin.x, Pmin.y, Pmax.x, Pmax.y)
#define fn_begin_sub_char(Psw,Pmin,Pmax) (*sp_globals.begin_sub_char)(SPD_GARG2 Psw.x, Psw.y, Pmin.x, Pmin.y, Pmax.x, Pmax.y)
#define fn_end_sub_char() (*sp_globals.end_sub_char)(SPD_GARG1)
#define fn_end_char() (*sp_globals.end_char)(SPD_GARG1)
#define fn_line(P1) (*sp_globals.line)(SPD_GARG2 P1.x, P1.y)
#define fn_end_contour() (*sp_globals.end_contour)(SPD_GARG1)
#define fn_begin_contour(P0,fmt) (*sp_globals.begin_contour)(SPD_GARG2 P0.x, P0.y, fmt)
#define fn_curve(P1,P2,P3,depth) (*sp_globals.curve)(SPD_GARG2 P1.x, P1.y, P2.x , P2.y, P3.x, P3.y, depth)

#if INCL_MULTIDEV

#define open_bitmap(xmin, xmax, ymin, ymax) (*sp_globals.bitmap_device.p_open_bitmap)(SPD_GARG2 xmin, xmax, ymin, ymax)
#define set_bitmap_bits(y, xbit1, xbit2) (*sp_globals.bitmap_device.p_set_bits)(SPD_GARG2 y, xbit1, xbit2)
#define close_bitmap() (*sp_globals.bitmap_device.p_close_bitmap)(SPD_GARG1)

#define open_outline(x_set_width, y_set_width, xmin, xmax, ymin, ymax) (*sp_globals.outline_device.p_open_outline)(SPD_GARG2 x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define start_sub_char() (*sp_globals.outline_device.p_start_sub_char)(SPD_GARG1)
#define end_sub_char() (*sp_globals.outline_device.p_end_sub_char)(SPD_GARG1)
#define start_contour(x,y,outside) (*sp_globals.outline_device.p_start_contour)(SPD_GARG2 x,y,outside)
#define curve_to(x1,y1,x2,y2,x3,y3) (*sp_globals.outline_device.p_curve)(SPD_GARG2 x1,y1,x2,y2,x3,y3)
#define line_to(x,y) (*sp_globals.outline_device.p_line)(SPD_GARG2 x,y)
#define close_contour() (*sp_globals.outline_device.p_close_contour)(SPD_GARG1)
#define close_outline() (*sp_globals.outline_device.p_close_outline)(SPD_GARG1)

#else

#define open_bitmap(xmin, xmax, ymin, ymax) sp_open_bitmap(SPD_GARG2 xmin, xmax, ymin, ymax)
#define set_bitmap_bits(y, xbit1, xbit2) sp_set_bitmap_bits(SPD_GARG2 y, xbit1, xbit2)
#define close_bitmap() sp_close_bitmap(SPD_GARG1)

#define open_outline(x_set_width, y_set_width, xmin, xmax, ymin, ymax) sp_open_outline(SPD_GARG2 x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define start_sub_char() sp_start_sub_char(SPD_GARG1)
#define end_sub_char() sp_end_sub_char(SPD_GARG1)
#define start_contour(x,y,outside) sp_start_contour(SPD_GARG2 x,y,outside)
#define curve_to(x1,y1,x2,y2,x3,y3) sp_curve_to(SPD_GARG2 x1,y1,x2,y2,x3,y3)
#define line_to(x,y) sp_line_to(SPD_GARG2 x,y)
#define close_contour() sp_close_contour(SPD_GARG1)
#define close_outline() sp_close_outline(SPD_GARG1)

#endif
