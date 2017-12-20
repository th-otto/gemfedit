/*
 * GEM resource C output of spdview
 *
 * created by ORCS 2.16
 */

#include <portab.h>

#ifdef OS_WINDOWS
#  include <portaes.h>
#  define SHORT _WORD
#  ifdef __WIN32__
#    define _WORD signed short
#  else
#    define _WORD signed int
 #   pragma option -zE_FARDATA
#  endif
#else
#  ifdef __TURBOC__
#    include <portaes.h>
#    define CP (_WORD *)
#  endif
#endif

#ifdef OS_UNIX
#  include <portaes.h>
#  define SHORT _WORD
#else
#  ifdef __GNUC__
#    ifndef __PORTAES_H__
#      if __GNUC__ < 4
#        include <aesbind.h>
#        ifndef _WORD
#          define _WORD int
#        endif
#        define CP (char *)
#      else
#        include <mt_gem.h>
#        ifndef _WORD
#          define _WORD short
#        endif
#        define CP (short *)
#      endif
#      define CW (short *)
#    endif
#  endif
#endif


#ifdef __SOZOBONX__
#  include <xgemfast.h>
#else
#  ifdef SOZOBON
#    include <aes.h>
#  endif
#endif

#ifdef MEGAMAX
#  include <gembind.h>
#  include <gemdefs.h>
#  include <obdefs.h>
#  define _WORD int
#  define SHORT int
#endif

#ifndef OS_NORMAL
#  define OS_NORMAL 0x0000
#endif
#ifndef OS_SELECTED
#  define OS_SELECTED 0x0001
#endif
#ifndef OS_CROSSED
#  define OS_CROSSED 0x0002
#endif
#ifndef OS_CHECKED
#  define OS_CHECKED 0x0004
#endif
#ifndef OS_DISABLED
#  define OS_DISABLED 0x0008
#endif
#ifndef OS_OUTLINED
#  define OS_OUTLINED 0x0010
#endif
#ifndef OS_SHADOWED
#  define OS_SHADOWED 0x0020
#endif
#ifndef OS_WHITEBAK
#  define OS_WHITEBAK 0x0040
#endif
#ifndef OS_DRAW3D
#  define OS_DRAW3D 0x0080
#endif

#ifndef OF_NONE
#  define OF_NONE 0x0000
#endif
#ifndef OF_SELECTABLE
#  define OF_SELECTABLE 0x0001
#endif
#ifndef OF_DEFAULT
#  define OF_DEFAULT 0x0002
#endif
#ifndef OF_EXIT
#  define OF_EXIT 0x0004
#endif
#ifndef OF_EDITABLE
#  define OF_EDITABLE 0x0008
#endif
#ifndef OF_RBUTTON
#  define OF_RBUTTON 0x0010
#endif
#ifndef OF_LASTOB
#  define OF_LASTOB 0x0020
#endif
#ifndef OF_TOUCHEXIT
#  define OF_TOUCHEXIT 0x0040
#endif
#ifndef OF_HIDETREE
#  define OF_HIDETREE 0x0080
#endif
#ifndef OF_INDIRECT
#  define OF_INDIRECT 0x0100
#endif
#ifndef OF_FL3DIND
#  define OF_FL3DIND 0x0200
#endif
#ifndef OF_FL3DBAK
#  define OF_FL3DBAK 0x0400
#endif
#ifndef OF_FL3DACT
#  define OF_FL3DACT 0x0600
#endif
#ifndef OF_MOVEABLE
#  define OF_MOVEABLE 0x0800
#endif
#ifndef OF_POPUP
#  define OF_POPUP 0x1000
#endif

#ifndef R_CICONBLK
#  define R_CICONBLK 17
#endif
#ifndef R_CICON
#  define R_CICON 18
#endif

#ifndef G_SWBUTTON
#  define G_SWBUTTON 34
#endif
#ifndef G_POPUP
#  define G_POPUP 35
#endif
#ifndef G_EDIT
#  define G_EDIT 37
#endif
#ifndef G_SHORTCUT
#  define G_SHORTCUT 38
#endif
#ifndef G_SLIST
#  define G_SLIST 39
#endif
#ifndef G_EXTBOX
#  define G_EXTBOX 40
#endif
#ifndef G_OBLINK
#  define G_OBLINK 41
#endif

#ifndef _WORD
#  ifdef WORD
#    define _WORD WORD
#  else
#    define _WORD short
#  endif
#endif

#ifndef _UBYTE
#  define _UBYTE char
#endif

#ifndef _LONG
#  ifdef LONG
#    define _LONG LONG
#  else
#    define _LONG long
#  endif
#endif

#ifndef _LONG_PTR
#  define _LONG_PTR _LONG
#endif

#ifndef C_UNION
#ifdef __PORTAES_H__
#  define C_UNION(x) { (_LONG_PTR)(x) }
#endif
#ifdef __GEMLIB__
#  define C_UNION(x) { (_LONG_PTR)(x) }
#endif
#ifdef __PUREC__
#  define C_UNION(x) { (_LONG_PTR)(x) }
#endif
#ifdef __ALCYON__
#  define C_UNION(x) x
#endif
#endif
#ifndef C_UNION
#  define C_UNION(x) (_LONG_PTR)(x)
#endif

#ifndef SHORT
#  define SHORT short
#endif

#ifndef CP
#  define CP (SHORT *)
#endif

#ifndef CW
#  define CW (_WORD *)
#endif


#undef RSC_STATIC_FILE
#define RSC_STATIC_FILE 1

#include "spdview.h"

#ifndef RSC_NAMED_FUNCTIONS
#  define RSC_NAMED_FUNCTIONS 0
#endif

#ifndef __ALCYON__
#undef defRSHInit
#undef defRSHInitBit
#undef defRSHInitStr
#ifndef RsArraySize
#define RsArraySize(array) (_WORD)(sizeof(array)/sizeof(array[0]))
#define RsPtrArraySize(type, array) (type *)array, RsArraySize(array)
#endif
#define defRSHInit( aa, bb ) RSHInit( aa, bb, RsPtrArraySize(OBJECT *, rs_trindex), RsArraySize(rs_object) )
#define defRSHInitBit( aa, bb ) RSHInitBit( aa, bb, RsPtrArraySize(BITBLK *, rs_frimg) )
#define defRSHInitStr( aa, bb ) RSHInitStr( aa, bb, RsPtrArraySize(_UBYTE *, rs_frstr) )
#endif

#ifdef __STDC__
#ifndef GetTextSize
extern _VOID GetTextSize(_WORD *_width, _WORD *_height);
#endif
#ifndef W_Cicon_Setpalette
extern _BOOL W_Cicon_Setpalette(_WORD *_palette);
#endif
#ifndef hrelease_objs
extern _VOID hrelease_objs(OBJECT *_ob, _WORD _num_objs);
#endif
#ifndef hfix_objs
extern _VOID *hfix_objs(RSHDR *_hdr, OBJECT *_ob, _WORD _num_objs);
#endif
#endif

#ifndef RLOCAL
#  if RSC_STATIC_FILE
#    ifdef LOCAL
#      define RLOCAL LOCAL
#    else
#      define RLOCAL static
#    endif
#  else
#    define RLOCAL
#  endif
#endif


#ifndef N_
#  define N_(x)
#endif


#if RSC_STATIC_FILE
#undef NUM_STRINGS
#undef NUM_BB
#undef NUM_IB
#undef NUM_CIB
#undef NUM_CIC
#undef NUM_TI
#undef NUM_FRSTR
#undef NUM_FRIMG
#undef NUM_OBS
#undef NUM_TREE
#undef NUM_UD
#define NUM_STRINGS 89
#define NUM_BB		0
#define NUM_IB		0
#define NUM_CIB     0
#define NUM_CIC     0
#define NUM_TI		13
#define NUM_FRSTR	8
#define NUM_FRIMG	0
#define NUM_OBS     65
#define NUM_TREE	4
#define NUM_UD		0
#endif


static char spdview_string_0[] = " Desk";
static char spdview_string_1[] = " File";
static char spdview_string_2[] = "  About...          ";
static char spdview_string_3[] = "---------------------";
static char spdview_string_4[] = "  Desk Accessory 1  ";
static char spdview_string_5[] = "  Desk Accessory 2  ";
static char spdview_string_6[] = "  Desk Accessory 3  ";
static char spdview_string_7[] = "  Desk Accessory 4  ";
static char spdview_string_8[] = "  Desk Accessory 5  ";
static char spdview_string_9[] = "  Desk Accessory 6  ";
static char spdview_string_10[] = "  Open  ^O";
static char spdview_string_11[] = "  Info  ^I";
static char spdview_string_12[] = "-----------";
static char spdview_string_13[] = "  Quit  ^Q";
static char spdview_string_14[] = "FONT PARAMETERS";
static char spdview_string_15[] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
static char spdview_string_16[] = "Font name        : ________________________________";
static char spdview_string_17[] = "X";
static char spdview_string_18[] = "@@@@@";
static char spdview_string_19[] = "Font ID          : _____";
static char spdview_string_20[] = "X";
static char spdview_string_21[] = "@@@@@";
static char spdview_string_22[] = "Point size       : _____";
static char spdview_string_23[] = "X";
static char spdview_string_24[] = "@@@";
static char spdview_string_25[] = "Max width         : ___";
static char spdview_string_26[] = "9";
static char spdview_string_27[] = "@@@";
static char spdview_string_28[] = "Cell height       : ___";
static char spdview_string_29[] = "9";
static char spdview_string_30[] = "@@@";
static char spdview_string_31[] = "Top line         : ___";
static char spdview_string_32[] = "9";
static char spdview_string_33[] = "@@@";
static char spdview_string_34[] = "Ascent line      : ___";
static char spdview_string_35[] = "9";
static char spdview_string_36[] = "System Font";
static char spdview_string_37[] = "@@@";
static char spdview_string_38[] = "Half line        : ___";
static char spdview_string_39[] = "9";
static char spdview_string_40[] = "Horizontal table";
static char spdview_string_41[] = "@@@";
static char spdview_string_42[] = "Descent line     : ___";
static char spdview_string_43[] = "9";
static char spdview_string_44[] = "Big-Endian";
static char spdview_string_45[] = "@@@";
static char spdview_string_46[] = "Bottom line      : ___";
static char spdview_string_47[] = "9";
static char spdview_string_48[] = "Monospaced";
static char spdview_string_49[] = "Compressed";
static char spdview_string_50[] = "@@@";
static char spdview_string_51[] = "First ascii code : ___";
static char spdview_string_52[] = "9";
static char spdview_string_53[] = "@@@";
static char spdview_string_54[] = "Last ascii code  : ___";
static char spdview_string_55[] = "9";
static char spdview_string_56[] = "OK";
static char spdview_string_57[] = " \001\002\003\004\005\006\007\b\t\n\013\f\r\016\017";
static char spdview_string_58[] = "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037";
static char spdview_string_59[] = " !\"#$%&\'()*+,-./";
static char spdview_string_60[] = "0123456789:;<=>?";
static char spdview_string_61[] = "@ABCDEFGHIJKLMNO";
static char spdview_string_62[] = "PQRSTUVWXYZ[\\]^_";
static char spdview_string_63[] = "`abcdefghijklmno";
static char spdview_string_64[] = "pqrstuvwxyz{|}~\177";
static char spdview_string_65[] = "\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217";
static char spdview_string_66[] = "\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237";
static char spdview_string_67[] = "\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257";
static char spdview_string_68[] = "\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277";
static char spdview_string_69[] = "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317";
static char spdview_string_70[] = "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337";
static char spdview_string_71[] = "\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357";
static char spdview_string_72[] = "\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";
static char spdview_string_73[] = "Font Displayer";
static char spdview_string_74[] = "For Atari ST/TT/Falcon";
static char spdview_string_75[] = "";
static char spdview_string_76[] = "";
static char spdview_string_77[] = "Written by Thorsten Otto";
static char spdview_string_78[] = "Version 1.0";
static char spdview_string_79[] = "Apr 20 2017";
static char spdview_string_80[] = "OK";
static char spdview_string_81[] = "[3][Cannot create window][Abort]";
static char spdview_string_82[] = "[3][Can\'t open|%s][Abort]";
static char spdview_string_83[] = "[3][Not a GEM font.][Abort]";
static char spdview_string_84[] = "[3][Not enough memory.][Abort]";
static char spdview_string_85[] = "Select Font";
static char spdview_string_86[] = "[1][Warning:|File may be truncated.][Continue]";
static char spdview_string_87[] = "[1][Warning:|Flag for horizontal table set,|but there is none.][Continue]";
static char spdview_string_88[] = "[1][Warning:|Horizontal offset table present,|but flag not set.][Continue]";


static char *rs_frstr[NUM_FRSTR] = {
	spdview_string_81,
	spdview_string_82,
	spdview_string_83,
	spdview_string_84,
	spdview_string_85,
	spdview_string_86,
	spdview_string_87,
	spdview_string_88
};


static TEDINFO rs_tedinfo[NUM_TI] = {
	{ spdview_string_15, spdview_string_16, spdview_string_17, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 33,52 }, /* FONT_NAME */
	{ spdview_string_18, spdview_string_19, spdview_string_20, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 6,25 }, /* FONT_ID */
	{ spdview_string_21, spdview_string_22, spdview_string_23, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 6,25 }, /* FONT_POINT */
	{ spdview_string_24, spdview_string_25, spdview_string_26, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,24 }, /* FONT_WIDTH */
	{ spdview_string_27, spdview_string_28, spdview_string_29, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,24 }, /* FONT_HEIGHT */
	{ spdview_string_30, spdview_string_31, spdview_string_32, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_TOP */
	{ spdview_string_33, spdview_string_34, spdview_string_35, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_ASCENT */
	{ spdview_string_37, spdview_string_38, spdview_string_39, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_HALF */
	{ spdview_string_41, spdview_string_42, spdview_string_43, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_DESCENT */
	{ spdview_string_45, spdview_string_46, spdview_string_47, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_BOTTOM */
	{ spdview_string_50, spdview_string_51, spdview_string_52, IBM, 0, TE_LEFT, 0x1180, 0x0, 0, 4,23 }, /* FONT_FIRST_ADE */
	{ spdview_string_53, spdview_string_54, spdview_string_55, IBM, 0, TE_LEFT, 0x1180, 0x0, 0, 4,23 }, /* FONT_LAST_ADE */
	{ spdview_string_74, spdview_string_75, spdview_string_76, SMALL, 0, TE_LEFT, 0x1B00, 0x0, -1, 23,1 }
};


static OBJECT rs_object[NUM_OBS] = {
/* MAINMENU */

	{ -1, 1, 5, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 0,0, 160,25 },
	{ 5, 2, 2, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0x1100L), 0,0, 160,513 },
	{ 1, 3, 4, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 2,0, 12,769 },
	{ 4, -1, -1, G_TITLE, OF_NONE, OS_NORMAL, C_UNION(spdview_string_0), 0,0, 6,769 }, /* TDESK */
	{ 2, -1, -1, G_TITLE, OF_NONE, OS_NORMAL, C_UNION(spdview_string_1), 6,0, 6,769 }, /* TFILE */
	{ 0, 6, 15, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 0,769, 160,19 },
	{ 15, 7, 14, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0xFF1100L), 2,0, 21,8 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_2), 0,0, 21,1 }, /* ABOUT */
	{ 9, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(spdview_string_3), 0,1, 21,1 },
	{ 10, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_4), 0,2, 21,1 },
	{ 11, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_5), 0,3, 21,1 },
	{ 12, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_6), 0,4, 21,1 },
	{ 13, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_7), 0,5, 21,1 },
	{ 14, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_8), 0,6, 21,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_9), 0,7, 21,1 },
	{ 5, 16, 19, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0xFF1100L), 8,0, 11,4 },
	{ 17, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_10), 0,0, 11,1 }, /* FOPEN */
	{ 18, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_11), 0,1, 11,1 }, /* FINFO */
	{ 19, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(spdview_string_12), 0,2, 11,1 },
	{ 15, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(spdview_string_13), 0,3, 11,1 }, /* FQUIT */

/* FONT_PARAMS */

	{ -1, 1, 19, G_BOX, OF_FL3DBAK, OS_OUTLINED, C_UNION(0x21100L), 4,1, 54,16 },
	{ 2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_14), 20,1, 15,1 },
	{ 3, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[0]), 2,3, 51,1 }, /* FONT_NAME */
	{ 4, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[1]), 2,4, 24,1 }, /* FONT_ID */
	{ 5, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[2]), 2,5, 24,1 }, /* FONT_POINT */
	{ 6, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[3]), 28,5, 23,1 }, /* FONT_WIDTH */
	{ 7, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[4]), 28,6, 23,1 }, /* FONT_HEIGHT */
	{ 8, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[5]), 2,7, 22,1 }, /* FONT_TOP */
	{ 9, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[6]), 2,8, 22,1 }, /* FONT_ASCENT */
	{ 10, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(spdview_string_36), 28,8, 22,1 }, /* FONT_SYSTEM */
	{ 11, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[7]), 2,9, 22,1 }, /* FONT_HALF */
	{ 12, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(spdview_string_40), 28,9, 22,1 }, /* FONT_HORTABLE */
	{ 13, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[8]), 2,10, 22,1 }, /* FONT_DESCENT */
	{ 14, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(spdview_string_44), 28,10, 22,1 }, /* FONT_BIGENDIAN */
	{ 15, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[9]), 2,11, 22,1 }, /* FONT_BOTTOM */
	{ 16, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(spdview_string_48), 28,11, 22,1 }, /* FONT_MONOSPACED */
	{ 17, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(spdview_string_49), 28,12, 22,1 }, /* FONT_COMPRESSED */
	{ 18, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[10]), 2,13, 22,1 }, /* FONT_FIRST_ADE */
	{ 19, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[11]), 2,14, 22,1 }, /* FONT_LAST_ADE */
	{ 0, -1, -1, G_BUTTON, 0x627, OS_NORMAL, C_UNION(spdview_string_56), 44,14, 8,1 }, /* FONT_OK */

/* PANEL */

	{ -1, 1, 1, G_BOX, OF_FL3DBAK, OS_NORMAL, C_UNION(0x74001100L), 0,0, 24,20 },
	{ 0, 2, 17, G_BOX, OF_NONE, OS_OUTLINED, C_UNION(0x21100L), 2,1, 20,18 },
	{ 3, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_57), 2,1, 16,1 },
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_58), 2,2, 16,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_59), 2,3, 16,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_60), 2,4, 16,1 },
	{ 7, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_61), 2,5, 16,1 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_62), 2,6, 16,1 },
	{ 9, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_63), 2,7, 16,1 },
	{ 10, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_64), 2,8, 16,1 },
	{ 11, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_65), 2,9, 16,1 },
	{ 12, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_66), 2,10, 16,1 },
	{ 13, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_67), 2,11, 16,1 },
	{ 14, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_68), 2,12, 16,1 },
	{ 15, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_69), 2,13, 16,1 },
	{ 16, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_70), 2,14, 16,1 },
	{ 17, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_71), 2,15, 16,1 },
	{ 1, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(spdview_string_72), 2,16, 16,1 },

/* ABOUT_DIALOG */

	{ -1, 1, 6, G_BOX, OF_FL3DBAK, OS_OUTLINED, C_UNION(0x21100L), 0,0, 32,13 },
	{ 2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_73), 3,1, 14,1 },
	{ 3, -1, -1, G_TEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[12]), 3,2, 1040,1 },
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_77), 3,4, 24,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_78), 3,6, 11,1 }, /* ABOUT_VERSION */
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(spdview_string_79), 3,8, 11,1 }, /* ABOUT_DATE */
	{ 0, -1, -1, G_BUTTON, 0x627, OS_NORMAL, C_UNION(spdview_string_80), 22,11, 8,1 }
};


static OBJECT *rs_trindex[NUM_TREE] = {
	&rs_object[0], /* MAINMENU */
	&rs_object[20], /* FONT_PARAMS */
	&rs_object[40], /* PANEL */
	&rs_object[58] /* ABOUT_DIALOG */
};





#if RSC_STATIC_FILE

#if RSC_NAMED_FUNCTIONS
#ifdef __STDC__
_WORD spdview_rsc_load(void)
#else
_WORD spdview_rsc_load()
#endif
{
#ifndef RSC_HAS_PALETTE
#  define RSC_HAS_PALETTE 0
#endif
#ifndef RSC_USE_PALETTE
#  define RSC_USE_PALETTE 0
#endif
#if RSC_HAS_PALETTE || RSC_USE_PALETTE
	W_Cicon_Setpalette(&rgb_palette[0][0]);
#endif
#if NUM_OBS != 0
	{
		_WORD Obj;
		OBJECT *tree;
		_WORD wchar, hchar;
		GetTextSize(&wchar, &hchar);
		for (Obj = 0, tree = rs_object; Obj < NUM_OBS; Obj++, tree++)
		{
			tree->ob_x = wchar * (tree->ob_x & 0xff) + (tree->ob_x >> 8);
			tree->ob_y = hchar * (tree->ob_y & 0xff) + (tree->ob_y >> 8);
			tree->ob_width = wchar * (tree->ob_width & 0xff) + (tree->ob_width >> 8);
			tree->ob_height = hchar * (tree->ob_height & 0xff) + (tree->ob_height >> 8);
		}
		hfix_objs(NULL, rs_object, NUM_OBS);
	}
#endif
	return 1;
}


#ifdef __STDC__
_WORD spdview_rsc_gaddr(_WORD type, _WORD idx, void *gaddr)
#else
_WORD spdview_rsc_gaddr(type, idx, gaddr)
_WORD type;
_WORD idx;
void *gaddr;
#endif
{
	switch (type)
	{
#if NUM_TREE != 0
	case R_TREE:
		if (idx < 0 || idx >= NUM_TREE)
			return 0;
		*((OBJECT **)gaddr) = rs_trindex[idx];
		break;
#endif
#if NUM_OBS != 0
	case R_OBJECT:
		if (idx < 0 || idx >= NUM_OBS)
			return 0;
		*((OBJECT **)gaddr) = &rs_object[idx];
		break;
#endif
#if NUM_TI != 0
	case R_TEDINFO:
		if (idx < 0 || idx >= NUM_TI)
			return 0;
		*((TEDINFO **)gaddr) = &rs_tedinfo[idx];
		break;
#endif
#if NUM_IB != 0
	case R_ICONBLK:
		if (idx < 0 || idx >= NUM_IB)
			return 0;
		*((ICONBLK **)gaddr) = &rs_iconblk[idx];
		break;
#endif
#if NUM_BB != 0
	case R_BITBLK:
		if (idx < 0 || idx >= NUM_BB)
			return 0;
		*((BITBLK **)gaddr) = &rs_bitblk[idx];
		break;
#endif
#if NUM_FRSTR != 0
	case R_STRING:
		if (idx < 0 || idx >= NUM_FRSTR)
			return 0;
		*((char **)gaddr) = (char *)(rs_frstr[idx]);
		break;
#endif
#if NUM_FRIMG != 0
	case R_IMAGEDATA:
		if (idx < 0 || idx >= NUM_FRIMG)
			return 0;
		*((BITBLK **)gaddr) = rs_frimg[idx];
		break;
#endif
#if NUM_OBS != 0
	case R_OBSPEC:
		if (idx < 0 || idx >= NUM_OBS)
			return 0;
		*((_LONG **)gaddr) = &rs_object[idx].ob_spec.index;
		break;
#endif
#if NUM_TI != 0
	case R_TEPTEXT:
		if (idx < 0 || idx >= NUM_TI)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_tedinfo[idx].te_ptext);
		break;
#endif
#if NUM_TI != 0
	case R_TEPTMPLT:
		if (idx < 0 || idx >= NUM_TI)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_tedinfo[idx].te_ptmplt);
		break;
#endif
#if NUM_TI != 0
	case R_TEPVALID:
		if (idx < 0 || idx >= NUM_TI)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_tedinfo[idx].te_pvalid);
		break;
#endif
#if NUM_IB != 0
	case R_IBPMASK:
		if (idx < 0 || idx >= NUM_IB)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_iconblk[idx].ib_pmask);
		break;
#endif
#if NUM_IB != 0
	case R_IBPDATA:
		if (idx < 0 || idx >= NUM_IB)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_iconblk[idx].ib_pdata);
		break;
#endif
#if NUM_IB != 0
	case R_IBPTEXT:
		if (idx < 0 || idx >= NUM_IB)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_iconblk[idx].ib_ptext);
		break;
#endif
#if NUM_BB != 0
	case R_BIPDATA:
		if (idx < 0 || idx >= NUM_BB)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_bitblk[idx].bi_pdata);
		break;
#endif
#if NUM_FRSTR != 0
	case R_FRSTR:
		if (idx < 0 || idx >= NUM_FRSTR)
			return 0;
		*((char ***)gaddr) = (char **)(&rs_frstr[idx]);
		break;
#endif
#if NUM_FRIMG != 0
	case R_FRIMG:
		if (idx < 0 || idx >= NUM_FRIMG)
			return 0;
		*((BITBLK ***)gaddr) = &rs_frimg[idx];
		break;
#endif
	default:
		return 0;
	}
	return 1;
}


#ifdef __STDC__
_WORD spdview_rsc_free(void)
#else
_WORD spdview_rsc_free()
#endif
{
#if NUM_OBS != 0
	hrelease_objs(rs_object, NUM_OBS);
#endif
	return 1;
}

#endif /* RSC_NAMED_FUNCTIONS */

#else /* !RSC_STATIC_FILE */
int rs_numstrings = 89;
int rs_numfrstr = 8;

int rs_nuser = 0;
int rs_numimages = 0;
int rs_numbb = 0;
int rs_numfrimg = 0;
int rs_numib = 0;
int rs_numcib = 0;
int rs_numti = 13;
int rs_numobs = 65;
int rs_numtree = 4;

char rs_name[] = "spdview.rsc";

int _rsc_format = 2; /* RSC_FORM_SOURCE2 */
#endif /* RSC_STATIC_FILE */
