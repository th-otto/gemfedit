/*
 * resource set indices for spdview
 *
 * created by ORCS 2.16
 */

/*
 * Number of Strings:        95
 * Number of Bitblks:        0
 * Number of Iconblks:       0
 * Number of Color Iconblks: 0
 * Number of Color Icons:    0
 * Number of Tedinfos:       11
 * Number of Free Strings:   10
 * Number of Free Images:    0
 * Number of Objects:        74
 * Number of Trees:          4
 * Number of Userblks:       0
 * Number of Images:         0
 * Total file size:          4272
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
#define NUM_STRINGS 95
#define NUM_FRSTR 10
#define NUM_UD 0
#define NUM_IMAGES 0
#define NUM_BB 0
#define NUM_FRIMG 0
#define NUM_IB 0
#define NUM_CIB 0
#define NUM_TI 11
#define NUM_OBS 74
#define NUM_TREE 4
#endif



#define MAINMENU                           0 /* menu */
#define TDESK                              3 /* TITLE in tree MAINMENU */
#define TFILE                              4 /* TITLE in tree MAINMENU */
#define ABOUT                              8 /* STRING in tree MAINMENU */
#define FOPEN                             17 /* STRING in tree MAINMENU */
#define FINFO                             18 /* STRING in tree MAINMENU */
#define FQUIT                             20 /* STRING in tree MAINMENU */
#define POINTS_6                          22 /* STRING in tree MAINMENU */
#define POINTS_8                          23 /* STRING in tree MAINMENU */
#define POINTS_9                          24 /* STRING in tree MAINMENU */
#define POINTS_10                         25 /* STRING in tree MAINMENU */
#define POINTS_12                         26 /* STRING in tree MAINMENU */
#define POINTS_14                         27 /* STRING in tree MAINMENU */
#define POINTS_16                         28 /* STRING in tree MAINMENU */
#define POINTS_18                         29 /* STRING in tree MAINMENU */
#define POINTS_24                         30 /* STRING in tree MAINMENU */
#define POINTS_36                         31 /* STRING in tree MAINMENU */
#define POINTS_48                         32 /* STRING in tree MAINMENU */
#define POINTS_64                         33 /* STRING in tree MAINMENU */
#define POINTS_72                         34 /* STRING in tree MAINMENU */

#define FONT_PARAMS                        1 /* form/dialog */
#define FONT_NAME                          2 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_SHORT_NAME                    3 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_FACE_NAME                     4 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_ID                            5 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_OK                            6 /* BUTTON in tree FONT_PARAMS */
#define FONT_FORM                          7 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_DATE                          8 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_LAYOUT_NAME                   9 /* FTEXT in tree FONT_PARAMS */ /* max len 56 */
#define FONT_CHARS_LAYOUT                 10 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_CHARS_FONT                   11 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */
#define FONT_FIRST_INDEX                  12 /* FTEXT in tree FONT_PARAMS */ /* max len 5 */

#define PANEL                              2 /* unknown form */

#define ABOUT_DIALOG                       3 /* form/dialog */
#define ABOUT_VERSION_LABEL                4 /* STRING in tree ABOUT_DIALOG */
#define ABOUT_VERSION                      5 /* STRING in tree ABOUT_DIALOG */
#define ABOUT_DATE                         6 /* STRING in tree ABOUT_DIALOG */

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

#define AL_NO_GLYPHS                       9 /* Alert string */
/* [3][No characters defined in font.][Abort] */




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
