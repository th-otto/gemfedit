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

#ifndef _SPEEDO_H_
#define _SPEEDO_H_

/***************************** S P E E D O . H *******************************
 ****************************************************************************/

/*****  USER OPTIONS OVERRIDE DEFAULTS ******/
#include "useropt.h"

#include <stdint.h>

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

/*****  CONFIGURATION DEFINITIONS *****/

#ifndef INCL_CLIPPING
#define INCL_CLIPPING 0		    /* 0 indicates CLIPPING code is not compiled in */
#endif

#ifndef INCL_SQUEEZING
#define INCL_SQUEEZING 0		/* 0 indicates SQUEEZE code is not compiled in */
#endif

/* 1 to include extended font support */
#ifndef INCL_EXT
#define INCL_EXT       1
#endif

/* 1 to include intelligent scaling support */
#ifndef INCL_RULES
#define INCL_RULES     1
#endif

/* 1 to include blackwriter output support */
#ifndef INCL_BLACK
#define INCL_BLACK     1
#endif

/* 1 to include screen writeroutput support */
#ifndef INCL_SCREEN
#define INCL_SCREEN     0
#endif

/* 1 to include outline output support */
#ifndef INCL_OUTLINE
#define INCL_OUTLINE     0
#endif

/* 1 to include 2d blackwriter output support */
#ifndef INCL_2D
#define INCL_2D          0
#endif

/* 1 to include user defined output module support */
#ifndef INCL_USEROUT
#define INCL_USEROUT      0
#endif

/* 1 to include load char data support */
#ifndef INCL_LCD
#define INCL_LCD       1
#endif

/* 1 to include imported width support */
#ifndef INCL_ISW
#define INCL_ISW       0
#endif

/* 1 to include metrics support */
#ifndef INCL_METRICS
#define INCL_METRICS   1
#endif

/* 1 to include multiple output device support */
#ifndef INCL_MULTIDEV
#define INCL_MULTIDEV  0
#endif

/* 1 to allocate small intercept lists */
#ifndef SHORT_LISTS
#define SHORT_LISTS 1
#endif

/* 1 to include plaid data monitoring */
#ifndef INCL_PLAID_OUT
#define INCL_PLAID_OUT 0
#endif

#define MODE_BLACK   0
#define MODE_SCREEN  1
#define MODE_OUTLINE 2
#define MODE_2D      3

#ifdef DYNAMIC_ALLOC
#if DYNAMIC_ALLOC
#define STATIC_ALLOC 0
#endif
#endif

#ifdef REENTRANT_ALLOC
#if REENTRANT_ALLOC
#define STATIC_ALLOC 0
#endif
#endif

#ifndef STATIC_ALLOC
#define STATIC_ALLOC 1
#endif

#ifndef DYNAMIC_ALLOC
#define DYNAMIC_ALLOC 0
#endif

#ifndef REENTRANT_ALLOC
#define REENTRANT_ALLOC 0
#endif

/*****  TYPE  DEFINITIONS *****/

#ifndef STDEF
#ifndef SPD_BMAP

typedef int8_t fix7;

typedef   uint8_t  ufix8;
#ifndef VFONT
typedef   uint8_t  boolean;
#endif
#endif

typedef   int16_t    fix15;

typedef   uint16_t   ufix16;

#ifndef __PORTVDI_H__
typedef   int32_t    fix31;
#endif

typedef   uint32_t   ufix32;
#endif

/***** GENERAL CONSTANTS *****/

#ifndef FALSE
#define  FALSE     0
#define  TRUE      1
#endif

#ifndef NULL
#include <stddef.h>
#endif

#undef MAX
#define MAX(h, i) ((h) > (i) ? (h) : (i))

#undef MIN
#define MIN(l, o) ((l) < (o) ? (l) : (o))

#define  BIT0           0x01
#define  BIT1           0x02
#define  BIT2           0x04
#define  BIT3           0x08
#define  BIT4           0x10
#define  BIT5           0x20
#define  BIT6           0x40
#define  BIT7           0x80

#if INCL_EXT

#define  MAX_CONSTR      750            /* Max constraints (incl 4 dummies) */
#define  MAX_CTRL_ZONES  256            /* Max number of controlled orus */
#define  MAX_INT_ZONES   256            /* Max number of interpolation zones */

#else
/* Compact fonts only supported */

#define  MAX_CONSTR      512            /* Max constraints (incl 4 dummies) */
#define  MAX_CTRL_ZONES   64            /* Max number of controlled orus */
#define  MAX_INT_ZONES    64            /* Max number of interpolation zones */

#endif

#define  SCALE_SHIFT   12               /* Binary point positiion for scale values */
#define  SCALE_RND   2048               /* Rounding bit for scaling transformation */
#define  ONE_SCALE   4096               /* Unity scale value */

#ifdef INCL_SCREEN
/* constants used by Screenwriter module */
#define LEFT_INT 1                      /* left intercept */
#define END_INT 2                       /* last intercept */
#define FRACTION 0xFC                   /* fractional portion of intercept type list */
#endif

/* constants used by SQUEEZEing code */
#define EM_TOP 764
#define EM_BOT -236

/*****  STRUCTURE DEFINITIONS *****/

typedef struct speedo_global_data SPEEDO_GLOBALS;

#if STATIC_ALLOC
extern SPEEDO_GLOBALS sp_globals;
#define sp_intercepts sp_globals
#define sp_plaid sp_globals
#define SPD_PROTO_DECL1 void
#define SPD_PROTO_DECL2
#define SPD_GARG
#define SPD_GARGS
#define SPD_GUNUSED
#elif DYNAMIC_ALLOC
extern SPEEDO_GLOBALS *sp_global_ptr;

#define sp_globals (*sp_global_ptr)
#define sp_intercepts (*sp_global_ptr)
#define sp_plaid (*sp_global_ptr)
#define SPD_PROTO_DECL1 void
#define SPD_PROTO_DECL2
#define SPD_GARG
#define SPD_GARGS
#define SPD_GUNUSED
#elif REENTRANT_ALLOC
#define sp_globals (*sp_global_ptr)
#define sp_intercepts (*(*sp_global_ptr).intercepts)
#define sp_plaid (*(*sp_global_ptr).plaid)
#define SPD_PROTO_DECL1 SPEEDO_GLOBALS *sp_global_ptr
#define SPD_PROTO_DECL2 SPD_PROTO_DECL1,
#define SPD_GARG sp_global_ptr
#define SPD_GARGS SPD_GARG,
#define SPD_GUNUSED (void)(sp_global_ptr);
#else
#define sp_globals (*sp_global_ptr)
#define sp_intercepts (*sp_global_ptr)
#define sp_plaid (*sp_global_ptr)
#define SPD_PROTO_DECL1 SPEEDO_GLOBALS *sp_global_ptr
#define SPD_PROTO_DECL2 SPD_PROTO_DECL1,
#define SPD_GARG sp_global_ptr
#define SPD_GARGS SPD_GARG,
#define SPD_GUNUSED (void)(sp_global_ptr);
#endif

/* Buffer descriptor */
typedef struct buff_tag
{
	ufix8 *org;                 /* Pointer to start of buffer */
	ufix32 no_bytes;            /* Size of buffer in bytes */
} buff_t;

/* Constraint data state */
typedef struct constr_tag {
	ufix8 *org;                 /* Pointer to first byte in constr data  */
	ufix16 font_id;             /* Font id for calculated data           */
	fix15 xppo;                 /* X pixels per oru for calculated data  */
	fix15 yppo;                 /* Y pixels per oru for calculated data  */
	boolean font_id_valid;      /* TRUE if font id valid                 */
	boolean data_valid;         /* TRUE if calculated data valid         */
	boolean active;             /* TRUE if constraints enabled           */
} constr_t;

/* Kerning control block */
typedef struct kern_tag {
	ufix8 *tkorg;               /* First byte of track kerning data      */
	ufix8 *pkorg;               /* First byte of pair kerning data       */
	fix15 no_tracks;            /* Number of kerning tracks              */
	fix15 no_pairs;             /* Number of kerning pairs               */
} kern_t;

/* Specs structure for sp_set_specs/fw_set_specs */
typedef struct specs_tag {
	fix31 xxmult;               /* Coeff of X orus to compute X pix      */
	fix31 xymult;               /* Coeff of Y orus to compute X pix      */
	fix31 xoffset;              /* Constant to compute X pix             */
	fix31 yxmult;               /* Coeff of X orus to compute Y pix      */
	fix31 yymult;               /* Coeff of Y orus to compute Y pix      */
	fix31 yoffset;              /* Constant to compute Y pix             */
	fix15 output_mode;          /* Output module selector */
	ufix32 flags;               /* Mode flags:                           */
                                /*   Bit  0 - 2: not used                */
                                /*   Bit  3: Send curves to output module*/
                                /*   Bit  4: Use linear scaling if set   */
                                /*   Bit  5: Inhibit constraint table    */
                                /*   Bit  6: Import set width if set     */
                                /*   Bit  7:   not used                  */
                                /*   Bit  8: Squeeze left if set         */
                                /*   Bit  9: Squeeze right if set        */
                                /*   Bit 10: Squeeze top if set          */
                                /*   Bit 11: Squeeze bottom if set       */
                                /*   Bit 12: Clip left if set            */
                                /*   Bit 13: Clip right if set           */
                                /*   Bit 14: Clip top if set             */
                                /*   Bit 15: Clip bottom if set          */
                                /*   Bits 16-31   not used               */
	void *out_info;             /* information for output module         */
} specs_t;

/* Transformation control block */
typedef struct tcb_tag {
	fix15 xxmult;               /* Linear coeff of Xorus to compute Xpix */
	fix15 xymult;               /* Linear coeff of Yorus to compute Xpix */
	fix31 xoffset;              /* Linear constant to compute Xpix       */
	fix15 yxmult;               /* Linear coeff of Xorus to compute Ypix */
	fix15 yymult;               /* Linear coeff of Yorus to compute Ypix */
	fix31 yoffset;              /* Linear constant to compute Ypix       */
	fix15 xppo;                 /* Pixels per oru in X dimension of char */
	fix15 yppo;                 /* Pixels per oru in Y dimension of char */
	fix15 xpos;                 /* Origin in X dimension of character    */
	fix15 ypos;                 /* Origin in Y dimension of character    */
	ufix16 xtype;               /* Transformation type for X oru coords  */
	ufix16 ytype;               /* Transformation type for Y oru coords  */
	ufix16 xmode;               /* Transformation mode for X oru coords  */
	ufix16 ymode;               /* Transformation mode for Y oru coords  */
#if INCL_SCREEN
	fix15 mirror;               /* Transformation creates mirror image   */
#endif
} tcb_t;

/* Point in device space */
typedef struct point_tag {
	fix15 x;                    /* X coord of point (shifted pixels)     */
	fix15 y;                    /* Y coord of point (shifted pixels)     */
} point_t;

typedef struct band_tag {
	fix15 band_max;
	fix15 band_min;
	fix15 band_array_offset;
	fix15 band_floor;
	fix15 band_ceiling;
} band_t;

typedef struct bbox_tag {
	fix31 xmin;
	fix31 xmax;
	fix31 ymin;
	fix31 ymax;
} bbox_t;

typedef struct _glyphinfo_t {
	fix15 ascent;
	fix15 descent;
	fix15 lbearing;
	fix15 rbearing;
	fix15 off_vert;
	uint16_t width;
	uint16_t height;
	fix31 xmin;
	fix31 ymin;
	fix31 xmax;
	fix31 ymax;
} glyphinfo_t;

#if SHORT_LISTS
#define  MAX_INTERCEPTS  256      /* Max storage for intercepts */
typedef ufix8 cdr_t;              /* 8 bit links in intercept chains */
#else
#define  MAX_INTERCEPTS 1000      /* Max storage for intercepts */
typedef ufix16 cdr_t;             /* 16 bit links in intercept chains */
#endif

#if REENTRANT_ALLOC

typedef struct intercepts_tag
{
	fix15 car[MAX_INTERCEPTS];
	cdr_t cdr[MAX_INTERCEPTS];
#if INCL_SCREEN
	ufix8 inttype[MAX_INTERCEPTS];
	ufix8 leftedge;
	ufix16 fracpix;
#endif
} intercepts_t;

typedef struct plaid_tag
{
	fix15 orus[MAX_CTRL_ZONES];         /* Controlled coordinate table (orus) */
#if INCL_RULES
	fix15 pix[MAX_CTRL_ZONES];          /* Controlled coordinate table (sub-pixels) */
	fix15 mult[MAX_INT_ZONES];          /* Interpolation multiplier table */
	fix31 offset[MAX_INT_ZONES];        /* Interpolation offset table */
#endif
} plaid_t;
#endif

#if INCL_MULTIDEV
typedef struct bitmap_tag {
	void (*p_open_bitmap)(SPD_PROTO_DECL2 fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize);
	void (*p_set_bits)(SPD_PROTO_DECL2 fix15 y, fix15 xbit1, fix15 xbit2);
	void (*p_close_bitmap)(SPD_PROTO_DECL1);
} bitmap_t;

typedef struct outline_tag {
	void (*p_open_outline)(SPD_PROTO_DECL2 fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax);
	void (*p_start_sub_char)(SPD_PROTO_DECL1);
	void (*p_end_sub_char)(SPD_PROTO_DECL1);
	void (*p_start_contour)(SPD_PROTO_DECL2 fix31 x, fix31 y, boolean outside);
	void (*p_curve)(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3);
	void (*p_line)(SPD_PROTO_DECL2 fix31 x, fix31 y);
	void (*p_close_contour)(SPD_PROTO_DECL1);
	void (*p_close_outline)(SPD_PROTO_DECL1);
} outline_t;
#endif

/* ---------------------------------------------------*/
/****  MAIN GLOBAL DATA STRUCTURE, SPEEDO_GLOBALS *****/

struct speedo_global_data {
	/*  do_char.c data definitions */
#if INCL_METRICS
	kern_t kern;                /* Kerning control block */
#endif
	point_t Psw;                /* End of escapement vector (1/65536 pixel units) */

#if INCL_LCD
	fix15 cb_offset;            /* Offset to sub-char data in char buffer */
#endif

	/* do_trns.c data definitions */
	point_t P0;                 /* Current point (sub-pixels) */
	fix15 x_orus;               /* Current X argument (orus) */
	fix15 y_orus;               /* Current Y argument (orus) */
	fix15 x_pix;                /* Current X argument (sub-pixels) */
	fix15 y_pix;                /* Current Y argument (sub-pixels) */
	ufix8 x_int;                /* Current X interpolation zone */
	ufix8 y_int;                /* Current Y interpolation zone */

#if INCL_MULTIDEV && INCL_OUTLINE
	outline_t outline_device;
	boolean outline_device_set;
#endif

#if INCL_BLACK || INCL_SCREEN || INCL_2D
#if INCL_MULTIDEV
	bitmap_t bitmap_device;
	boolean bitmap_device_set;
#endif
	band_t y_band;              /* Y current band(whole pixels) */

	struct set_width_tag
	{
		fix31 x;
		fix31 y;
	} set_width;                /* Character escapement vector */

	boolean first_pass;         /* TRUE during first pass thru outline data */
	boolean extents_running;    /* T if extent accumulation for each vector */
	fix15 x0_spxl;              /* X coord of current point (sub pixels) */
	fix15 y0_spxl;              /* Y coord of current point (sub pixels) */
	fix15 y_pxl;                /* Y coord of current point (whole pixels) */
#if REENTRANT_ALLOC
	intercepts_t *intercepts;
#else
	fix15 car[MAX_INTERCEPTS];  /* Data field of intercept storage */
	cdr_t cdr[MAX_INTERCEPTS];  /* Link field of intercept storage */
#if INCL_SCREEN
	ufix8 inttype[MAX_INTERCEPTS];
	ufix8 leftedge;
	ufix16 fracpix;
#endif
#endif
	fix15 bmap_xmin;            /* Min X value (sub-pixel units) */
	fix15 bmap_xmax;            /* Max X value (sub-pixel units) */
	fix15 bmap_ymin;            /* Min Y value (sub-pixel units) */
	fix15 bmap_ymax;            /* Max Y value (sub-pixel units) */
	fix15 no_y_lists;           /* Number of active intercept lists */
	fix15 first_offset;         /* Index of first active list cell */
	fix15 next_offset;          /* Index of next free list cell */
	boolean intercept_oflo;     /* TRUE if intercepts data lost */
#endif

	/* bounding box now used by all output modules, including outline */
	fix15 xmin;                 /* Min X value in whole character */
	fix15 xmax;                 /* Max X value in whole character */
	fix15 ymin;                 /* Min Y value in whole character */
	fix15 ymax;                 /* Max Y value in whole character */

#if INCL_2D
	fix15 no_x_lists;           /* Number of active x intercept lists */
	band_t x_band;              /* X current band(whole pixels) */
	boolean x_scan_active;      /* X scan flag during scan conversion */
#endif

	/* reset.c data definitions */
	ufix16 key32;               /* Decryption keys 3,2 combined */
	ufix8 key4;                 /* Decryption key 4 */
	ufix8 key6;                 /* Decryption key 6 */
	ufix8 key7;                 /* Decryption key 7 */
	ufix8 key8;                 /* Decryption key 8 */

	/* set_spcs.c data definitions */
	buff_t font;                /* font buffer structure */
	long font_buff_size;        /* Number of bytes loaded in font buffer */
	ufix8 *pchar_dir;           /* Pointer to character directory */
	fix15 first_char_idx;       /* Index to first character in font */
	fix15 no_chars_avail;       /* Total characters in font layout */
	fix15 orus_per_em;          /* Outline resolution */
	fix15 metric_resolution;    /* metric resolution for setwidths, kerning pairs
	                               (defaults to orus_per_em) */
	tcb_t tcb0;                 /* Top level transformation control block */
	
	boolean specs_valid;        /* TRUE if fw_set_specs() successful */
	
	fix15 depth_adj;            /* Curve splitting depth adjustment */
	fix15 thresh;               /* Scan conversion threshold (sub-pixels) */
	boolean normal;             /* TRUE if 0 obl and mult of 90 deg rot  */
	
	fix15 multshift;            /* Fixed point shift for multipliers */
	fix15 pixshift;             /* Fixed point shift for sub-pixels */
	fix15 poshift;              /* Left shift from pixel to output format */
	fix15 mpshift;              /* Fixed point shift for mult to sub-pixels */
	fix31 multrnd;              /* 0.5 in multiplier units */
	fix15 pixrnd;               /* 0.5 in sub-pixel units */
	fix31 mprnd;                /* 0.5 sub-pixels in multiplier units */
	fix15 pixfix;               /* Mask to remove fractional pixels */
	fix15 onepix;               /* 1.0 pixels in sub-pixel units */

	boolean (*init_out)(SPD_PROTO_DECL2 specs_t *specsarg);
	boolean (*begin_char)(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
	void (*begin_sub_char)(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 max);
	void (*begin_contour)(SPD_PROTO_DECL2 fix31 x1, fix31 y1, boolean outside); 
	void (*curve)(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth);  
	void (*line)(SPD_PROTO_DECL2 fix31 x1, fix31 y1);               
	void (*end_contour)(SPD_PROTO_DECL1);
	void (*end_sub_char)(SPD_PROTO_DECL1);
	boolean (*end_char)(SPD_PROTO_DECL1);

	specs_t specs;              /* copy specs onto stack */
	ufix8 *font_org;            /* Pointer to start of font data */
	ufix8 *hdr2_org;            /* Pointer to start of private header data */

	/* set_trns.c data definitions */
	tcb_t tcb;                  /* Current transformation control block */
	ufix8 Y_edge_org;           /* Index to first Y controlled coordinate */
	ufix8 Y_int_org;            /* Index to first Y interpolation zone */
	fix31 rnd_xmin;             /* rounded out value of xmin for int-char spac. fix */

#if REENTRANT_ALLOC
	plaid_t *plaid;
#else
	fix15 orus[MAX_CTRL_ZONES]; /* Controlled coordinate table (orus) */
#if INCL_RULES
	fix15 pix[MAX_CTRL_ZONES];  /* Controlled coordinate table (sub-pixels) */
	fix15 mult[MAX_INT_ZONES];  /* Interpolation multiplier table */
	fix31 offset[MAX_INT_ZONES]; /* Interpolation offset table */
#endif
#endif

	fix15 no_X_orus;            /* Number of X controlled coordinates */
	fix15 no_Y_orus;            /* Number of Y controlled coordinates */
	ufix16 Y_constr_org;        /* Origin of constraint table in font data */

#if INCL_RULES
	constr_t constr;            /* Constraint data state */
	boolean c_act[MAX_CONSTR];  /* TRUE if constraint currently active */
	fix15 c_pix[MAX_CONSTR];    /* Size of constrained zone if active */
#endif
#if INCL_ISW
	boolean import_setwidth_act; /* boolean to indicate imported setwidth */
	boolean isw_modified_constants;
	ufix32 imported_width;      /* value of imported setwidth */
	fix15 isw_xmax;             /* maximum oru value for constants */
#endif
#if INCL_SQUEEZING || INCL_ISW
	fix15 setwidth_orus;        /* setwidth value in orus */
	/* bounding box in orus for squeezing */
	fix15 bbox_xmin_orus;       /* X minimum in orus */
	fix15 bbox_xmax_orus;       /* X maximum in orus */
	fix15 bbox_ymin_orus;       /* Y minimum in orus */
	fix15 bbox_ymax_orus;       /* Y maximum in orus */
#endif
#if INCL_SQUEEZING
	boolean squeezing_compound; /* flag to indicate a compound character */
#endif
#if INCL_CLIPPING
	fix31 clip_xmax;
	fix31 clip_ymax;
	fix31 clip_xmin;
	fix31 clip_ymin;
#endif
};

/***********************************************************************************
 *
 *  Speedo global data structure allocation 
 *
 ***********************************************************************************/

/***** PUBLIC FONT HEADER OFFSET CONSTANTS  *****/
#define  FH_FMVER    0      /* U   D4.0 CR LF NULL NULL  8 bytes            */
#define  FH_FNTSZ    8      /* U   Font size (bytes) 4 bytes                */
#define  FH_FBFSZ   12      /* U   Min font buffer size (bytes) 4 bytes     */
#define  FH_CBFSZ   16      /* U   Min char buffer size (bytes) 2 bytes     */
#define  FH_HEDSZ   18      /* U   Header size (bytes) 2 bytes              */
#define  FH_FNTID   20      /* U   Source Font ID  2 bytes                  */
#define  FH_SFVNR   22      /* U   Source Font Version Number  2 bytes      */
#define  FH_FNTNM   24      /* U   Source Font Name  70 bytes               */
#define  FH_MDATE   94      /* U   Manufacturing Date  10 bytes             */
#define  FH_LAYNM  104      /* U   Layout Name  70 bytes                    */
#define  FH_CPYRT  174      /* U   Copyright Notice  78 bytes               */
#define  FH_NCHRL  252      /* U   Number of Chars in Layout  2 bytes       */
#define  FH_NCHRF  254      /* U   Total Number of Chars in Font  2 bytes   */
#define  FH_FCHRF  256      /* U   Index of first char in Font  2 bytes     */
#define  FH_NKTKS  258      /* U   Number of kerning tracks in font 2 bytes */
#define  FH_NKPRS  260      /* U   Number of kerning pairs in font 2 bytes  */
#define  FH_FLAGS  262      /* U   Font flags 1 byte:                       */
                            /*       Bit 0: Extended font                   */
                            /*       Bit 1: not used                        */
                            /*       Bit 2: not used                        */
                            /*       Bit 3: not used                        */
                            /*       Bit 4: not used                        */
                            /*       Bit 5: not used                        */
                            /*       Bit 6: not used                        */
                            /*       Bit 7: not used                        */
#define  FH_CLFGS  263      /* U   Classification flags 1 byte:             */
                            /*       Bit 0: Italic                          */
                            /*       Bit 1: Monospace                       */
                            /*       Bit 2: Serif                           */
                            /*       Bit 3: Display                         */
                            /*       Bit 4: not used                        */
                            /*       Bit 5: not used                        */
                            /*       Bit 6: not used                        */
                            /*       Bit 7: not used                        */
#define  FH_FAMCL  264      /* U   Family Classification 1 byte:            */
                            /*       0:  Don't care                         */
                            /*       1:  Serif                              */
                            /*       2:  Sans serif                         */
                            /*       3:  Monospace                          */
                            /*       4:  Script or calligraphic             */
                            /*       5:  Decorative                         */
                            /*       6-255: not used                        */
#define  FH_FRMCL  265      /* U   Font form Classification 1 byte:         */
                            /*       Bits 0-3 (width type):                 */
                            /*         0-3:   not used                      */
                            /*         4:     Condensed                     */
                            /*         5:     not used                      */
                            /*         6:     Semi-condensed                */
                            /*         7:     not used                      */
                            /*         8:     Normal                        */
                            /*         9:     not used                      */
                            /*        10:     Semi-expanded                 */
                            /*        11:     not used                      */
                            /*        12:     Expanded                      */
                            /*        13-15:  not used                      */
                            /*       Bits 4-7 (Weight):                     */
                            /*         0:   not used                        */
                            /*         1:   Thin                            */
                            /*         2:   Ultralight                      */
                            /*         3:   Extralight                      */
                            /*         4:   Light                           */
                            /*         5:   Book                            */
                            /*         6:   Normal                          */
                            /*         7:   Medium                          */
                            /*         8:   Semibold                        */
                            /*         9:   Demibold                        */
                            /*         10:  Bold                            */
                            /*         11:  Extrabold                       */
                            /*         12:  Ultrabold                       */
                            /*         13:  Heavy                           */
                            /*         14:  Black                           */
                            /*         15-16: not used                      */
#define  FH_SFNTN  266      /* U   Short Font Name  32 bytes                */
#define  FH_SFACN  298      /* U   Short Face Name  16 bytes                */
#define  FH_FNTFM  314      /* U   Font form 14 bytes                       */
#define  FH_ITANG  328      /* U   Italic angle 2 bytes (1/256th deg)       */
#define  FH_ORUPM  330      /* U   Number of ORUs per em  2 bytes           */
#define  FH_WDWTH  332      /* U   Width of Wordspace  2 bytes              */
#define  FH_EMWTH  334      /* U   Width of Emspace  2 bytes                */
#define  FH_ENWTH  336      /* U   Width of Enspace  2 bytes                */
#define  FH_TNWTH  338      /* U   Width of Thinspace  2 bytes              */
#define  FH_FGWTH  340      /* U   Width of Figspace  2 bytes               */
#define  FH_FXMIN  342      /* U   Font-wide min X value  2 bytes           */
#define  FH_FYMIN  344      /* U   Font-wide min Y value  2 bytes           */
#define  FH_FXMAX  346      /* U   Font-wide max X value  2 bytes           */
#define  FH_FYMAX  348      /* U   Font-wide max Y value  2 bytes           */
#define  FH_ULPOS  350      /* U   Underline position 2 bytes               */
#define  FH_ULTHK  352      /* U   Underline thickness 2 bytes              */
#define  FH_SMCTR  354      /* U   Small caps transformation 6 bytes        */
#define  FH_DPSTR  360      /* U   Display sups transformation 6 bytes      */
#define  FH_FNSTR  366      /* U   Footnote sups transformation 6 bytes     */
#define  FH_ALSTR  372      /* U   Alpha sups transformation 6 bytes        */
#define  FH_CMITR  378      /* U   Chemical infs transformation 6 bytes     */
#define  FH_SNMTR  384      /* U   Small nums transformation 6 bytes        */
#define  FH_SDNTR  390      /* U   Small denoms transformation 6 bytes      */
#define  FH_MNMTR  396      /* U   Medium nums transformation 6 bytes       */
#define  FH_MDNTR  402      /* U   Medium denoms transformation 6 bytes     */
#define  FH_LNMTR  408      /* U   Large nums transformation 6 bytes        */
#define  FH_LDNTR  414      /* U   Large denoms transformation 6 bytes      */
                            /*     Transformation data format:              */
                            /*       Y position 2 bytes                     */
                            /*       X scale 2 bytes (1/4096ths)            */
                            /*       Y scale 2 bytes (1/4096ths)            */
#define  SIZE_FW (FH_LDNTR + 6)  /* size of nominal font header */
#define  EXP_FH_METRES SIZE_FW   /* offset to expansion field metric resolution (optional) */



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

/***********************************************************************************
 *
 *  Speedo function declarations - use prototypes if available
 *
 ***********************************************************************************/

/*  do_char.c functions */
#define SP_UNDEFINED 0
#define UNDEFINED 0xffff
ufix16 sp_get_char_id(SPD_PROTO_DECL2 ufix16 char_index);
boolean sp_make_char(SPD_PROTO_DECL2 ufix16 char_index);

#if INCL_ISW
fix31 sp_compute_isw_scale(SPD_PROTO_DECL1);
boolean sp_make_char_isw(SPD_PROTO_DECL2 ufix16 char_index, ufix32 imported_width);
#endif

#if INCL_METRICS
fix31 sp_get_char_width(SPD_PROTO_DECL2 ufix16 char_index);
fix15 sp_get_track_kern(SPD_PROTO_DECL2 fix15 track, fix15 point_size);
fix31 sp_get_pair_kern(SPD_PROTO_DECL2 ufix16 char_index1, ufix16 char_index2);
boolean sp_get_char_bbox(SPD_PROTO_DECL2 ufix16 char_index, bbox_t *bbox, boolean no_adj);
#endif

/* do_trns.c functions */
ufix8 *sp_read_bbox(SPD_PROTO_DECL2 ufix8 *pointer, point_t *pPmin, point_t *pPmax, boolean set_flag);
void sp_proc_outl_data(SPD_PROTO_DECL2 ufix8 *pointer);

/* out_blk.c functions */
#if INCL_BLACK
boolean sp_init_black(SPD_PROTO_DECL2 specs_t *specsarg);
boolean sp_begin_char_black(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_begin_contour_black(SPD_PROTO_DECL2 fix31 x1, fix31 y1, boolean outside);
void sp_line_black(SPD_PROTO_DECL2 fix31 x, fix31 y);
boolean sp_end_char_black(SPD_PROTO_DECL1);
#endif

/* out_scrn.c functions */
#if INCL_SCREEN
boolean sp_init_screen(SPD_PROTO_DECL2 specs_t *specsarg);
boolean sp_begin_char_screen(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_begin_contour_screen(SPD_PROTO_DECL2 fix31 x1, fix31 y1, boolean outside);
void sp_curve_screen(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth);
void sp_line_screen(SPD_PROTO_DECL2 fix31 x, fix31 y);
void sp_end_contour_screen(SPD_PROTO_DECL1);
boolean sp_end_char_screen(SPD_PROTO_DECL1);
#endif

/* out_outl.c functions */
#if INCL_OUTLINE
#if INCL_MULTIDEV
boolean sp_set_outline_device(SPD_PROTO_DECL2 outline_t *ofuncs, ufix16 size);
#endif


boolean sp_init_outline(SPD_PROTO_DECL2 specs_t *specsarg);
boolean sp_begin_char_outline(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_begin_sub_char_outline(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 max);
void sp_begin_contour_outline(SPD_PROTO_DECL2 fix31 x1, fix31 y1, boolean outside);
void sp_curve_outline(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth);
void sp_line_outline(SPD_PROTO_DECL2 fix31 x, fix31 y);
void sp_end_contour_outline(SPD_PROTO_DECL1);
void sp_end_sub_char_outline(SPD_PROTO_DECL1);
boolean sp_end_char_outline(SPD_PROTO_DECL1);
#endif

/* out_bl2d.c functions */
#if INCL_2D
boolean sp_init_2d(SPD_PROTO_DECL2 specs_t *specsarg);
boolean sp_begin_char_2d(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_begin_contour_2d(SPD_PROTO_DECL2 fix31 x1, fix31 y1, boolean outside);
void sp_line_2d(SPD_PROTO_DECL2 fix31 x, fix31 y);
boolean sp_end_char_2d(SPD_PROTO_DECL1);
#endif

/* out_util.c functions */
#if INCL_BLACK || INCL_SCREEN || INCL_2D

#if INCL_MULTIDEV
boolean sp_set_bitmap_device(SPD_PROTO_DECL2 bitmap_t *bfuncs, ufix16 size);
#endif

void sp_init_char_out(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_begin_sub_char_out(SPD_PROTO_DECL2 fix31 x, fix31 y, fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_curve_out(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3, fix15 depth);
void sp_end_contour_out(SPD_PROTO_DECL1);
void sp_end_sub_char_out(SPD_PROTO_DECL1);
void sp_init_intercepts_out(SPD_PROTO_DECL1);
void sp_restart_intercepts_out(SPD_PROTO_DECL1);
void sp_set_first_band_out(SPD_PROTO_DECL2 fix31 minx, fix31 miny, fix31 maxx, fix31 maxy);
void sp_reduce_band_size_out(SPD_PROTO_DECL1);
boolean sp_next_band_out(SPD_PROTO_DECL1);
#endif

#if INCL_USEROUT
boolean sp_init_userout(SPD_PROTO_DECL2 specs_t *specsarg);
#endif


/* reset.c functions */
void sp_reset(SPD_PROTO_DECL1);
void sp_set_key(SPD_PROTO_DECL2 const ufix8 *key);
void sp_reset_key(SPD_PROTO_DECL1);
const ufix8 *sp_get_key(SPD_PROTO_DECL2 const buff_t *font_buff);
ufix16 sp_get_cust_no(SPD_PROTO_DECL2 const buff_t *font_buff);

/* set_spcs.c functions */
boolean sp_set_specs(SPD_PROTO_DECL2 const specs_t *specsarg, const buff_t *font);
void sp_type_tcb(SPD_PROTO_DECL2 tcb_t *ptcb);

fix31 sp_read_long(SPD_PROTO_DECL2 ufix8 *pointer);
fix15 sp_read_word_u(SPD_PROTO_DECL2 ufix8 *pointer);

/* set_trns.c functions */
void sp_init_tcb(SPD_PROTO_DECL1);
void sp_scale_tcb(SPD_PROTO_DECL2 tcb_t *ptcb, fix15 x_pos, fix15 y_pos, fix15 x_scale, fix15 y_scale);
ufix8 *sp_plaid_tcb(SPD_PROTO_DECL2 ufix8 *pointer, ufix8 format);
ufix8 *sp_skip_interpolation_table(SPD_PROTO_DECL2 ufix8 *pointer, ufix8 format);
ufix8 *sp_skip_control_zone(SPD_PROTO_DECL2 ufix8 *pointer, ufix8 format);

ufix8 *sp_read_oru_table(SPD_PROTO_DECL2 ufix8 *pointer);
#if INCL_SQUEEZING
boolean sp_calculate_x_scale(SPD_PROTO_DECL2 fix31 *x_factor, fix31 *x_offset, fix15 no_x_ctrl_zones);
boolean sp_calculate_y_scale(SPD_PROTO_DECL2 fix31 *top_scale, fix31 *bottom_scale, fix15 first_y_zone, fix15 no_Y_ctrl_zones);
#endif


/* user defined functions */

void sp_report_error(SPD_PROTO_DECL2 fix15 n);
void sp_write_error(SPD_PROTO_DECL2 const char *fmt, ...);

#if INCL_BLACK || INCL_SCREEN || INCL_2D
void sp_open_bitmap(SPD_PROTO_DECL2 fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize);
void sp_set_bitmap_bits(SPD_PROTO_DECL2 fix15 y, fix15 xbit1, fix15 xbit2);
void sp_close_bitmap(SPD_PROTO_DECL1);
#endif

#if INCL_OUTLINE
void sp_open_outline(SPD_PROTO_DECL2 fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax);
void sp_start_sub_char(SPD_PROTO_DECL1);
void sp_end_sub_char(SPD_PROTO_DECL1);
void sp_start_contour(SPD_PROTO_DECL2 fix31 x, fix31 y, boolean outside);
void sp_curve_to(SPD_PROTO_DECL2 fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3);
void sp_line_to(SPD_PROTO_DECL2 fix31 x, fix31 y);
void sp_close_contour(SPD_PROTO_DECL1);
void sp_close_outline(SPD_PROTO_DECL1);
#endif

#if INCL_LCD
/* Load character data from font file */
boolean sp_load_char_data(SPD_PROTO_DECL2 long file_offset, fix15 no_bytes, fix15 cb_offset, buff_t *char_data);
#endif

#if INCL_PLAID_OUT
void sp_record_xint(SPD_PROTO_DECL2 fix15 int_num);            /* Record xint data */
void sp_record_yint(SPD_PROTO_DECL2 fix15 int_num);            /* Record yint data */
void sp_begin_plaid_data(SPD_PROTO_DECL1);         /* Signal start of plaid data */
void sp_begin_ctrl_zones(SPD_PROTO_DECL2 fix15, no_X_zones, fix15 no_Y_zones);         /* Signal start of control zones */
void sp_record_ctrl_zone(SPD_PROTO_DECL2 fix31 start, fix31 end, fix15 constr);         /* Record control zone data */
void sp_begin_int_zones(SPD_PROTO_DECL2 fix15 no_X_int_zones, fix15 no_Y_int_zones);          /* Signal start of interpolation zones */
void sp_record_int_zone(SPD_PROTO_DECL2 fix31 start, fix31 end);          /* Record interpolation zone data */
void sp_end_plaid_data(SPD_PROTO_DECL1);           /* Signal end of plaid data */
#endif

#endif /* _SPEEDO_H_ */
