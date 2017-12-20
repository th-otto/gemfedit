/*
 * resource set indices for spdview
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        90
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       13
 * Number of Free Strings:   9
 * Number of Free Images:    0
 * Number of Objects:        65
 * Number of Trees:          4
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          3458
 */

#undef RSC_NAME
#ifndef __ALCYON__
#define RSC_NAME "spdview"
#endif
#undef RSC_ID
#ifdef spdview
#define RSC_ID spdview
#else
#define RSC_ID 0
#endif

#ifndef RSC_STATIC_FILE
# define RSC_STATIC_FILE 0
#endif
#if !RSC_STATIC_FILE
#define NUM_STRINGS 90
#define NUM_FRSTR 9
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 13
#define NUM_OBS 65
#define NUM_TREE 4
#endif



#define MAINMENU                           0 /* menu */
#define TDESK                              3 /* TITLE in tree MAINMENU */
#define TFILE                              4 /* TITLE in tree MAINMENU */
#define ABOUT                              7 /* STRING in tree MAINMENU */
#define FOPEN                             16 /* STRING in tree MAINMENU */
#define FINFO                             17 /* STRING in tree MAINMENU */
#define FQUIT                             19 /* STRING in tree MAINMENU */

#define FONT_PARAMS                        1 /* form/dialog */
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
#define FONT_OK                           19 /* BUTTON in tree FONT_PARAMS */

#define PANEL                              2 /* unknown form */

#define ABOUT_DIALOG                       3 /* form/dialog */
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

#define AL_TRUNCATED                       5 /* Alert string */
/* [1][Warning:|File may be truncated.][Continue] */

#define AL_MISSING_HOR_TABLE               6 /* Alert string */
/* [1][Warning:|Flag for horizontal table set,|but there is none.][Continue] */

#define AL_MISSING_HOR_TABLE_FLAG          7 /* Alert string */
/* [1][Warning:|Horizontal offset table present,|but flag not set.][Continue] */

#define AL_READERROR                       8 /* Alert string */
/* [3][Read error on file.][Abort] */




#ifdef __STDC__
#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif
extern _WORD spdview_rsc_load(void);
extern _WORD spdview_rsc_gaddr(_WORD type, _WORD idx, void *gaddr);
extern _WORD spdview_rsc_free(void);
#endif
