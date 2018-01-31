/*
 * resource set indices for gemfedit
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        122
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       13
 * Number of Free Strings:   24
 * Number of Free Images:    0
 * Number of Objects:        83
 * Number of Trees:          5
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          5374
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "gemfedit"
#endif
#undef RSC_ID
#ifdef gemfedit
#define RSC_ID gemfedit
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 122
#define NUM_FRSTR 24
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 13
#define NUM_OBS 83
#define NUM_TREE 5
#endif



#define MAINMENU                           0 /* menu */
#define TDESK                              3 /* TITLE in tree MAINMENU */
#define TFILE                              4 /* TITLE in tree MAINMENU */
#define ABOUT                              8 /* STRING in tree MAINMENU */
#define FOPEN                             17 /* STRING in tree MAINMENU */
#define FSAVE                             18 /* STRING in tree MAINMENU */
#define FINFO                             19 /* STRING in tree MAINMENU */
#define FEXPORTC                          21 /* STRING in tree MAINMENU */
#define FEXPORTTXT                        22 /* STRING in tree MAINMENU */
#define FIMPORTTXT                        23 /* STRING in tree MAINMENU */
#define FSYS_6X6                          25 /* STRING in tree MAINMENU */
#define FSYS_8X8                          26 /* STRING in tree MAINMENU */
#define FSYS_8X16                         27 /* STRING in tree MAINMENU */
#define FSYS_16X32                        28 /* STRING in tree MAINMENU */
#define FQUIT                             30 /* STRING in tree MAINMENU */
#define CHAR_NEXT                         32 /* STRING in tree MAINMENU */
#define CHAR_PREV                         33 /* STRING in tree MAINMENU */
#define CHAR_FIRST                        34 /* STRING in tree MAINMENU */
#define CHAR_LAST                         35 /* STRING in tree MAINMENU */

/* Zeichensatz */
#define CHARSET                            1 /* free form */

#define FONT_PARAMS                        2 /* form/dialog */
#define FONT_NAME                          2 /* FTEXT in tree FONT_PARAMS */ /* max len 32 */
#define FONT_ID                            3 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_POINT                         4 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_WIDTH                         5 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_HEIGHT                        6 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_TOP                           7 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_ASCENT                        8 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_SYSTEM                        9 /* BUTTON in tree FONT_PARAMS */
#define FONT_HALF                         10 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_HORTABLE                     11 /* BUTTON in tree FONT_PARAMS */
#define FONT_DESCENT                      12 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_BIGENDIAN                    13 /* BUTTON in tree FONT_PARAMS */
#define FONT_BOTTOM                       14 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_MONOSPACED                   15 /* BUTTON in tree FONT_PARAMS */
#define FONT_COMPRESSED                   16 /* BUTTON in tree FONT_PARAMS */
#define FONT_FIRST_ADE                    17 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_LAST_ADE                     18 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_CANCEL                       19 /* BUTTON in tree FONT_PARAMS */
#define FONT_OK                           20 /* BUTTON in tree FONT_PARAMS */

#define ABOUT_DIALOG                       3 /* form/dialog */
#define ABOUT_VERSION_LABEL                4 /* STRING in tree ABOUT_DIALOG */
#define ABOUT_DATE                         5 /* STRING in tree ABOUT_DIALOG */
#define ABOUT_VERSION                      7 /* STRING in tree ABOUT_DIALOG */

#define HELP_DIALOG                        4 /* form/dialog */

#define AL_NOWINDOW                        0 /* Alert string */
/* [3][Cannot create window][Abort] */

#define AL_FOPEN                           1 /* Alert string */
/* [3][Can't open|%s][Abort] */

#define AL_NOGEMFONT                       2 /* Alert string */
/* [3][Not a GEM font.][Abort] */

#define AL_NOMEM                           3 /* Alert string */
/* [3][Not enough memory.][Abort] */

#define SEL_FONT                           4 /* Free string */
/* Select Font */

#define AL_QUIT                            5 /* Alert string */
/* [2][Font has been changed.|Quit anyway?][No|Yes] */

#define AL_CHANGED                         6 /* Alert string */
/* [2][Font has been changed.|Load anyway?][No|Yes] */

#define SEL_OUTPUT                         7 /* Free string */
/* Select Output File */

#define AL_EXISTS                          8 /* Alert string */
/* [2][File already exists.|Overwrite?][No|Yes] */

#define AL_FCREATE                         9 /* Alert string */
/* [3][Can't create|%s][Abort] */

#define AL_NO_OFFTABLE                    10 /* Alert string */
/* [3][Fonts with horizontal offset|tables are not supported.][Abort] */

#define SEL_INPUT                         11 /* Free string */
/* Select Input File */

#define AL_FIMPORT                        12 /* Alert string */
/* [2][Error in file|%s|line %d.][Abort] */

#define AL_CHAR_RANGE                     13 /* Alert string */
/* [2][Wrong char range:|first = %u, last = %u][Abort] */

#define AL_FONT_SIZE                      14 /* Alert string */
/* [2][Unreasonable font size:|%ux%u.][Abort] */

#define AL_WRONG_CHAR                     15 /* Alert string */
/* [2][Wrong character number|%u at line %d.][Abort] */

#define AL_LINE_TOO_LONG                  16 /* Alert string */
/* [2][Bitmap line to long at line %d.][Abort] */

#define AL_DIFFERENT_LENGTH               17 /* Alert string */
/* [2][Bitmap lines of different|length at line %d.][Abort] */

#define AL_TRUNCATED                      18 /* Alert string */
/* [1][Warning:|File may be truncated.][Continue] */

#define AL_MISSING_HOR_TABLE              19 /* Alert string */
/* [1][Warning:|Flag for horizontal table set,|but there is none.][Continue] */

#define AL_MISSING_HOR_TABLE_FLAG         20 /* Alert string */
/* [1][Warning:|Horizontal offset table present,|but flag not set.][Continue] */

#define AL_COMPRESSED_SIZE                21 /* Alert string */
/* [3][Compressed size %lu larger|than uncompressed size %lu.][Abort] */

#define AL_ENDIAN_FLAG                    22 /* Alert string */
/* [1][Warning:|Wrong endian flag in header.][Continue] */

#define AL_FONTWIDTH                      23 /* Alert string */
/* [3][Maxiumum font width exceeded.][Abort] */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD gemfedit_rsc_load(void);
extern _WORD gemfedit_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD gemfedit_rsc_free(void);
#endif
