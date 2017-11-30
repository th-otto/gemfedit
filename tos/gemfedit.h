/*
 * resource set indices for gemfedit
 *
 * created by ORCS 2.15
 */

/*
 * Number of Strings:        113
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       13
 * Number of Free Strings:   18
 * Number of Free Images:    0
 * Number of Objects:        80
 * Number of Trees:          5
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          4854
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
#define NUM_STRINGS 113
#define NUM_FRSTR 18
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 13
#define NUM_OBS 80
#define NUM_TREE 5
#endif



#define MAINMENU                           0 /* menu */
#define TDESK                              3 /* TITLE in tree MAINMENU */
#define TFILE                              4 /* TITLE in tree MAINMENU */
#define ABOUT                              7 /* STRING in tree MAINMENU */
#define FOPEN                             16 /* STRING in tree MAINMENU */
#define FSAVE                             17 /* STRING in tree MAINMENU */
#define FINFO                             18 /* STRING in tree MAINMENU */
#define FEXPORTC                          20 /* STRING in tree MAINMENU */
#define FEXPORTTXT                        21 /* STRING in tree MAINMENU */
#define FIMPORTTXT                        22 /* STRING in tree MAINMENU */
#define FSYS_6X6                          24 /* STRING in tree MAINMENU */
#define FSYS_8X8                          25 /* STRING in tree MAINMENU */
#define FSYS_8X16                         26 /* STRING in tree MAINMENU */
#define FSYS_16X32                        27 /* STRING in tree MAINMENU */
#define FQUIT                             29 /* STRING in tree MAINMENU */

/* Zeichensatz */
#define CHARSET                            1 /* free form */

#define FONT_PARAMS                        2 /* form/dialog */
#define FONT_NAME                          2 /* FTEXT in tree FONT_PARAMS */ /* max len 32 */
#define FONT_ID                            3 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_POINT                         4 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_TOP                           5 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_WIDTH                         6 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_ASCENT                        7 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_HEIGHT                        8 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_HALF                          9 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_DESCENT                      10 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_BOTTOM                       11 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_FIRST_ADE                    12 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_LAST_ADE                     13 /* FTEXT in tree FONT_PARAMS */ /* max len 3 */
#define FONT_OK                           14 /* BUTTON in tree FONT_PARAMS */
#define FONT_CANCEL                       15 /* BUTTON in tree FONT_PARAMS */

#define PANEL                              3 /* unknown form */
#define PANEL_BG                           0 /* BOX in tree PANEL */
#define PANEL_BOX                          1 /* BOX in tree PANEL */
#define PANEL_FIRST                        2 /* STRING in tree PANEL */
#define PANEL_LAST                        17 /* STRING in tree PANEL */

#define ABOUT_DIALOG                       4 /* form/dialog */
#define ABOUT_VERSION                      4 /* STRING in tree ABOUT_DIALOG */
#define ABOUT_DATE                         5 /* STRING in tree ABOUT_DIALOG */

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
/* [2][Font has been changed.|Quit Anyway?][No|Yes] */

#define AL_CHANGED                         6 /* Alert string */
/* [2][Font has been changed.|Load Anyway?][No|Yes] */

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
