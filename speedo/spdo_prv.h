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

/*****  CONFIGURATION DEFINITIONS *****/


/***** PRIVATE FONT HEADER OFFSET CONSTANTS  *****/
#define  FH_ORUMX    0      /* U   Max ORU value  2 bytes                   */
#define  FH_PIXMX    2      /* U   Max Pixel value  2 bytes                 */
#define  FH_CUSNR    4      /* U   Customer Number  2 bytes                 */
#define  FH_OFFCD    6      /* E   Offset to Char Directory  3 bytes        */
#define  FH_OFCNS    9      /* E   Offset to Constraint Data  3 bytes       */
#define  FH_OFFTK   12      /* E   Offset to Track Kerning  3 bytes         */
#define  FH_OFFPK   15      /* E   Offset to Pair Kerning  3 bytes          */
#define  FH_OCHRD   18      /* E   Offset to Character Data  3 bytes        */
#define  FH_NBYTE   21      /* E   Number of Bytes in File  3 bytes         */


/***** MODE FLAGS CONSTANTS *****/
#define CURVES_OUT     0X0008  /* Output module accepts curves              */
#define BOGUS_MODE     0X0010  /* Linear scaling mode                       */
#define CONSTR_OFF     0X0020  /* Inhibit constraint table                  */
#define IMPORT_WIDTHS  0X0040  /* Imported width mode                       */
#define SQUEEZE_LEFT   0X0100  /* Squeeze left mode                         */
#define SQUEEZE_RIGHT  0X0200  /* Squeeze right mode                        */
#define SQUEEZE_TOP    0X0400  /* Squeeze top mode                          */
#define SQUEEZE_BOTTOM 0X0800  /* Squeeze bottom mode                       */
#define CLIP_LEFT      0X1000  /* Clip left mode                            */
#define CLIP_RIGHT     0X2000  /* Clip right mode                           */
#define CLIP_TOP       0X4000  /* Clip top mode                             */
#define CLIP_BOTTOM    0X8000  /* Clip bottom mode                          */


/***** MACRO DEFINITIONS *****/

#define SQUEEZE_MULT(A,B) (((fix31)A * (fix31)B) / (1 << 16))

#define NEXT_BYTE(A) (*(A)++)

#define NEXT_WORD(A) \
    ((fix15)(sp_globals.key32 ^ ((A) += 2, \
				 ((fix15)((A)[-1]) << 8) | (fix15)((A)[-2]) | \
				 ((A)[-1] & 0x80? ~0xFFFF : 0))))

#if INCL_EXT                       /* Extended fonts supported? */

#define NEXT_BYTES(A, B) \
    (((B = (ufix16)(*(A)++) ^ sp_globals.key7) >= 248)? \
     ((ufix16)(B & 0x07) << 8) + ((*(A)++) ^ sp_globals.key8) + 248: \
     B)

#else                              /* Compact fonts only supported? */

#define NEXT_BYTES(A, B) ((*(A)++) ^ sp_globals.key7)

#endif


#define NEXT_BYTE_U(A) (*(A)++) 

#define NEXT_WORD_U(A, B) \
    (fix15)(B = (*(A)++) << 8, (fix15)(*(A)++) + B)

#define NEXT_CHNDX(A, B) \
    ((B)? (ufix16)NEXT_WORD(A): (ufix16)NEXT_BYTE(A))

/* Multiply (fix15)X by (fix15)Y to produce (fix31)product */
#define MULT16(X, Y) \
    ((fix31)X * (fix31)Y)

/* Multiply (fix15)X by (fix15)MULT, add (fix31)OFFSET, 
 * shift right SHIFT bits to produce (fix15)result */
#define TRANS(X, MULT, OFFSET, SHIFT) \
    ((fix15)((((fix31)X * (fix31)MULT) + OFFSET) / (1 << SHIFT)))

/******************************************************************************
 *
 *      the following block of definitions redefines every function
 *      reference to be prefixed with an "sp_".  In addition, if this 
 *      is a reentrant version, the parameter sp_globals will be added
 *      as the first parameter.
 *
 *****************************************************************************/

#define fn_init_out(specsarg) (*sp_globals.init_out)(specsarg)  
#define fn_begin_char(Psw,Pmin,Pmax) (*sp_globals.begin_char)(Psw,Pmin,Pmax)
#define fn_begin_sub_char(Psw,Pmin,Pmax) (*sp_globals.begin_sub_char)(Psw,Pmin,Pmax)
#define fn_end_sub_char() (*sp_globals.end_sub_char)()
#define fn_end_char() (*sp_globals.end_char)()
#define fn_line(P1) (*sp_globals.line)(P1)
#define fn_end_contour() (*sp_globals.end_contour)()
#define fn_begin_contour(P0,fmt) (*sp_globals.begin_contour)(P0,fmt)
#define fn_curve(P1,P2,P3,depth) (*sp_globals.curve)(P1,P2,P3,depth)

#if INCL_MULTIDEV

#define open_bitmap(x_set_width, y_set_width, xmin, xmax, ymin, ymax) (*sp_globals.bitmap_device.p_open_bitmap)(x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define set_bitmap_bits(y, xbit1, xbit2) (*sp_globals.bitmap_device.p_set_bits)(y, xbit1, xbit2)
#define close_bitmap() (*sp_globals.bitmap_device.p_close_bitmap)()

#define open_outline(x_set_width, y_set_width, xmin, xmax, ymin, ymax) (*sp_globals.outline_device.p_open_outline)(x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define start_new_char() (*sp_globals.outline_device.p_start_char)()
#define start_contour(x,y,outside) (*sp_globals.outline_device.p_start_contour)(x,y,outside)
#define curve_to(x1,y1,x2,y2,x3,y3) (*sp_globals.outline_device.p_curve)(x1,y1,x2,y2,x3,y3)
#define line_to(x,y) (*sp_globals.outline_device.p_line)(x,y)
#define close_contour() (*sp_globals.outline_device.p_close_contour)()
#define close_outline() (*sp_globals.outline_device.p_close_outline)()

#else

#define open_bitmap(x_set_width, y_set_width, xmin, xmax, ymin, ymax) sp_open_bitmap(x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define set_bitmap_bits(y, xbit1, xbit2) sp_set_bitmap_bits(y, xbit1, xbit2)
#define close_bitmap() sp_close_bitmap()

#define open_outline(x_set_width, y_set_width, xmin, xmax, ymin, ymax) sp_open_outline(x_set_width, y_set_width, xmin, xmax, ymin, ymax)
#define start_new_char() sp_start_new_char()
#define start_contour(x,y,outside) sp_start_contour(x,y,outside)
#define curve_to(x1,y1,x2,y2,x3,y3) sp_curve_to(x1,y1,x2,y2,x3,y3)
#define line_to(x,y) sp_line_to(x,y)
#define close_contour() sp_close_contour()
#define close_outline() sp_close_outline()

#endif
