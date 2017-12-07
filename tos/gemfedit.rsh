/*
 * GEM resource C output of gemfedit
 *
 * created by ORCS 2.15
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

#include "gemfedit.h"

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
#define NUM_STRINGS 131
#define NUM_BB		0
#define NUM_IB		0
#define NUM_CIB     0
#define NUM_CIC     0
#define NUM_TI		13
#define NUM_FRSTR	23
#define NUM_FRIMG	0
#define NUM_OBS     94
#define NUM_TREE	6
#define NUM_UD		0
#endif


static char gemfedit_string_0[] = " Desk";
static char gemfedit_string_1[] = " File";
static char gemfedit_string_2[] = "  About...";
static char gemfedit_string_3[] = "---------------------";
static char gemfedit_string_4[] = "  Desk Accessory 1";
static char gemfedit_string_5[] = "  Desk Accessory 2";
static char gemfedit_string_6[] = "  Desk Accessory 3";
static char gemfedit_string_7[] = "  Desk Accessory 4";
static char gemfedit_string_8[] = "  Desk Accessory 5";
static char gemfedit_string_9[] = "  Desk Accessory 6";
static char gemfedit_string_10[] = "  Open           ^O";
static char gemfedit_string_11[] = "  Save           ^S";
static char gemfedit_string_12[] = "  Info           ^I";
static char gemfedit_string_13[] = "--------------------";
static char gemfedit_string_14[] = "  Export as C    ^E";
static char gemfedit_string_15[] = "  Export as TXT";
static char gemfedit_string_16[] = "  Import from TXT";
static char gemfedit_string_17[] = "--------------------";
static char gemfedit_string_18[] = "  Load system 6x6";
static char gemfedit_string_19[] = "  Load system 8x8";
static char gemfedit_string_20[] = "  Load system 8x16";
static char gemfedit_string_21[] = "  Load system 16x32";
static char gemfedit_string_22[] = "--------------------";
static char gemfedit_string_23[] = "  Quit           ^Q";
static char gemfedit_string_24[] = "  \001 \002 \003 \004 \005 \006 \007 \b \t \n \013 \f \r \016 \017 \020 \021 \022 \023 \024 \025 \026 \027 \030 \031 \032 \033 \034 \035 \036 \037";
static char gemfedit_string_25[] = "  ! \" # $ % & \' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ?";
static char gemfedit_string_26[] = "@ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ \\ ] ^ _";
static char gemfedit_string_27[] = "` a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~ \177";
static char gemfedit_string_28[] = "\200 \201 \202 \203 \204 \205 \206 \207 \210 \211 \212 \213 \214 \215 \216 \217 \220 \221 \222 \223 \224 \225 \226 \227 \230 \231 \232 \233 \234 \235 \236 \237";
static char gemfedit_string_29[] = "\240 \241 \242 \243 \244 \245 \246 \247 \250 \251 \252 \253 \254 \255 \256 \257 \260 \261 \262 \263 \264 \265 \266 \267 \270 \271 \272 \273 \274 \275 \276 \277";
static char gemfedit_string_30[] = "\300 \301 \302 \303 \304 \305 \306 \307 \310 \311 \312 \313 \314 \315 \316 \317 \320 \321 \322 \323 \324 \325 \326 \327 \330 \331 \332 \333 \334 \335 \336 \337";
static char gemfedit_string_31[] = "\340 \341 \342 \343 \344 \345 \346 \347 \350 \351 \352 \353 \354 \355 \356 \357 \360 \361 \362 \363 \364 \365 \366 \367 \370 \371 \372 \373 \374 \375 \376 \377";
static char gemfedit_string_32[] = "FONT PARAMETERS";
static char gemfedit_string_33[] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
static char gemfedit_string_34[] = "Font name        : ________________________________";
static char gemfedit_string_35[] = "X";
static char gemfedit_string_36[] = "@@@@@";
static char gemfedit_string_37[] = "Font ID          : _____";
static char gemfedit_string_38[] = "9";
static char gemfedit_string_39[] = "@@@@@";
static char gemfedit_string_40[] = "Point size       : _____";
static char gemfedit_string_41[] = "9";
static char gemfedit_string_42[] = "@@@";
static char gemfedit_string_43[] = "Max width         : ___";
static char gemfedit_string_44[] = "9";
static char gemfedit_string_45[] = "@@@";
static char gemfedit_string_46[] = "Cell height       : ___";
static char gemfedit_string_47[] = "9";
static char gemfedit_string_48[] = "@@@";
static char gemfedit_string_49[] = "Top line         : ___";
static char gemfedit_string_50[] = "9";
static char gemfedit_string_51[] = "@@@";
static char gemfedit_string_52[] = "Ascent line      : ___";
static char gemfedit_string_53[] = "9";
static char gemfedit_string_54[] = "System Font";
static char gemfedit_string_55[] = "@@@";
static char gemfedit_string_56[] = "Half line        : ___";
static char gemfedit_string_57[] = "9";
static char gemfedit_string_58[] = "Horizontal table";
static char gemfedit_string_59[] = "@@@";
static char gemfedit_string_60[] = "Descent line     : ___";
static char gemfedit_string_61[] = "9";
static char gemfedit_string_62[] = "Big-Endian";
static char gemfedit_string_63[] = "@@@";
static char gemfedit_string_64[] = "Bottom line      : ___";
static char gemfedit_string_65[] = "9";
static char gemfedit_string_66[] = "Monospaced";
static char gemfedit_string_67[] = "Compressed";
static char gemfedit_string_68[] = "@@@";
static char gemfedit_string_69[] = "First ascii code : ___";
static char gemfedit_string_70[] = "9";
static char gemfedit_string_71[] = "@@@";
static char gemfedit_string_72[] = "Last ascii code  : ___";
static char gemfedit_string_73[] = "9";
static char gemfedit_string_74[] = "Cancel";
static char gemfedit_string_75[] = "OK";
static char gemfedit_string_76[] = " \001\002\003\004\005\006\007\b\t\n\013\f\r\016\017";
static char gemfedit_string_77[] = "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037";
static char gemfedit_string_78[] = " !\"#$%&\'()*+,-./";
static char gemfedit_string_79[] = "0123456789:;<=>?";
static char gemfedit_string_80[] = "@ABCDEFGHIJKLMNO";
static char gemfedit_string_81[] = "PQRSTUVWXYZ[\\]^_";
static char gemfedit_string_82[] = "`abcdefghijklmno";
static char gemfedit_string_83[] = "pqrstuvwxyz{|}~\177";
static char gemfedit_string_84[] = "\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217";
static char gemfedit_string_85[] = "\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237";
static char gemfedit_string_86[] = "\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257";
static char gemfedit_string_87[] = "\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277";
static char gemfedit_string_88[] = "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317";
static char gemfedit_string_89[] = "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337";
static char gemfedit_string_90[] = "\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357";
static char gemfedit_string_91[] = "\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";
static char gemfedit_string_92[] = "Simple Font Editor";
static char gemfedit_string_93[] = "For Atari ST/TT/Falcon";
static char gemfedit_string_94[] = "";
static char gemfedit_string_95[] = "";
static char gemfedit_string_96[] = "Written by Thorsten Otto";
static char gemfedit_string_97[] = "Version 1.0";
static char gemfedit_string_98[] = "Nov 30 2017";
static char gemfedit_string_99[] = "OK";
static char gemfedit_string_100[] = "Keyboard shortcuts:";
static char gemfedit_string_101[] = "OK";
static char gemfedit_string_102[] = "Cursor up   : scale window up";
static char gemfedit_string_103[] = "Cursor down : scale window down";
static char gemfedit_string_104[] = "Cursor left : select previous char";
static char gemfedit_string_105[] = "Cursor right: select next char";
static char gemfedit_string_106[] = "Clicking in the font overview";
static char gemfedit_string_107[] = "will select the character.";
static char gemfedit_string_108[] = "[3][Cannot create window][Abort]";
static char gemfedit_string_109[] = "[3][Can\'t open|%s][Abort]";
static char gemfedit_string_110[] = "[3][Not a GEM font.][Abort]";
static char gemfedit_string_111[] = "[3][Not enough memory.][Abort]";
static char gemfedit_string_112[] = "Select Font";
static char gemfedit_string_113[] = "[2][Font has been changed.|Quit anyway?][No|Yes]";
static char gemfedit_string_114[] = "[2][Font has been changed.|Load anyway?][No|Yes]";
static char gemfedit_string_115[] = "Select Output File";
static char gemfedit_string_116[] = "[2][File already exists.|Overwrite?][No|Yes]";
static char gemfedit_string_117[] = "[3][Can\'t create|%s][Abort]";
static char gemfedit_string_118[] = "[3][Fonts with horizontal offset|tables are not supported.][Abort]";
static char gemfedit_string_119[] = "Select Input File";
static char gemfedit_string_120[] = "[2][Error in file|%s|line %d.][Abort]";
static char gemfedit_string_121[] = "[2][Wrong char range:|first = %u, last = %u][Abort]";
static char gemfedit_string_122[] = "[2][Unreasonable font size:|%ux%u.][Abort]";
static char gemfedit_string_123[] = "[2][Wrong character number|%u at line %d.][Abort]";
static char gemfedit_string_124[] = "[2][Bitmap line to long at line %d.][Abort]";
static char gemfedit_string_125[] = "[2][Bitmap lines of different|length at line %d.][Abort]";
static char gemfedit_string_126[] = "[1][Warning:|File may be truncated.][Continue]";
static char gemfedit_string_127[] = "[1][Warning:|Flag for horizontal table set,|but there is none.][Continue]";
static char gemfedit_string_128[] = "[1][Warning:|Horizontal offset table present,|but flag not set.][Continue]";
static char gemfedit_string_129[] = "[3][Compressed size %lu larger|than uncompressed size %lu.][Abort]";
static char gemfedit_string_130[] = "[1][Warning:|Wrong endian flag in header.][Continue]";


static char *rs_frstr[NUM_FRSTR] = {
	gemfedit_string_108,
	gemfedit_string_109,
	gemfedit_string_110,
	gemfedit_string_111,
	gemfedit_string_112,
	gemfedit_string_113,
	gemfedit_string_114,
	gemfedit_string_115,
	gemfedit_string_116,
	gemfedit_string_117,
	gemfedit_string_118,
	gemfedit_string_119,
	gemfedit_string_120,
	gemfedit_string_121,
	gemfedit_string_122,
	gemfedit_string_123,
	gemfedit_string_124,
	gemfedit_string_125,
	gemfedit_string_126,
	gemfedit_string_127,
	gemfedit_string_128,
	gemfedit_string_129,
	gemfedit_string_130
};


static TEDINFO rs_tedinfo[NUM_TI] = {
	{ gemfedit_string_33, gemfedit_string_34, gemfedit_string_35, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 33,52 }, /* FONT_NAME */
	{ gemfedit_string_36, gemfedit_string_37, gemfedit_string_38, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 6,25 }, /* FONT_ID */
	{ gemfedit_string_39, gemfedit_string_40, gemfedit_string_41, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 6,25 }, /* FONT_POINT */
	{ gemfedit_string_42, gemfedit_string_43, gemfedit_string_44, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,24 }, /* FONT_WIDTH */
	{ gemfedit_string_45, gemfedit_string_46, gemfedit_string_47, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,24 }, /* FONT_HEIGHT */
	{ gemfedit_string_48, gemfedit_string_49, gemfedit_string_50, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_TOP */
	{ gemfedit_string_51, gemfedit_string_52, gemfedit_string_53, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_ASCENT */
	{ gemfedit_string_55, gemfedit_string_56, gemfedit_string_57, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_HALF */
	{ gemfedit_string_59, gemfedit_string_60, gemfedit_string_61, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_DESCENT */
	{ gemfedit_string_63, gemfedit_string_64, gemfedit_string_65, IBM, 0, TE_LEFT, 0x1180, 0x0, -1, 4,23 }, /* FONT_BOTTOM */
	{ gemfedit_string_68, gemfedit_string_69, gemfedit_string_70, IBM, 0, TE_LEFT, 0x1180, 0x0, 0, 4,23 }, /* FONT_FIRST_ADE */
	{ gemfedit_string_71, gemfedit_string_72, gemfedit_string_73, IBM, 0, TE_LEFT, 0x1180, 0x0, 0, 4,23 }, /* FONT_LAST_ADE */
	{ gemfedit_string_93, gemfedit_string_94, gemfedit_string_95, SMALL, 0, TE_LEFT, 0x1B00, 0x0, -1, 23,1 }
};


static OBJECT rs_object[NUM_OBS] = {
/* MAINMENU */

	{ -1, 1, 5, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 0,0, 160,25 },
	{ 5, 2, 2, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0x1100L), 0,0, 160,513 },
	{ 1, 3, 4, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 2,0, 12,769 },
	{ 4, -1, -1, G_TITLE, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_0), 0,0, 6,769 }, /* TDESK */
	{ 2, -1, -1, G_TITLE, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_1), 6,0, 6,769 }, /* TFILE */
	{ 0, 6, 15, G_IBOX, OF_NONE, OS_NORMAL, C_UNION(0x0L), 0,769, 160,19 },
	{ 15, 7, 14, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0xFF1100L), 2,0, 21,8 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_2), 0,0, 21,1 }, /* ABOUT */
	{ 9, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(gemfedit_string_3), 0,1, 21,1 },
	{ 10, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_4), 0,2, 21,1 },
	{ 11, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_5), 0,3, 21,1 },
	{ 12, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_6), 0,4, 21,1 },
	{ 13, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_7), 0,5, 21,1 },
	{ 14, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_8), 0,6, 21,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_9), 0,7, 21,1 },
	{ 5, 16, 29, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0xFF1100L), 8,0, 20,14 },
	{ 17, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_10), 0,0, 20,1 }, /* FOPEN */
	{ 18, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_11), 0,1, 20,1 }, /* FSAVE */
	{ 19, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_12), 0,2, 20,1 }, /* FINFO */
	{ 20, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(gemfedit_string_13), 0,3, 20,1 },
	{ 21, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_14), 0,4, 20,1 }, /* FEXPORTC */
	{ 22, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_15), 0,5, 20,1 }, /* FEXPORTTXT */
	{ 23, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_16), 0,6, 20,1 }, /* FIMPORTTXT */
	{ 24, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(gemfedit_string_17), 0,7, 20,1 },
	{ 25, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_18), 0,8, 20,1 }, /* FSYS_6X6 */
	{ 26, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_19), 0,9, 20,1 }, /* FSYS_8X8 */
	{ 27, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_20), 0,10, 20,1 }, /* FSYS_8X16 */
	{ 28, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_21), 0,11, 20,1 }, /* FSYS_16X32 */
	{ 29, -1, -1, G_STRING, OF_NONE, OS_DISABLED, C_UNION(gemfedit_string_22), 0,12, 20,1 },
	{ 15, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(gemfedit_string_23), 0,13, 20,1 }, /* FQUIT */

/* CHARSET */

	{ -1, 1, 8, G_BOX, OF_NONE, OS_OUTLINED, C_UNION(0x21100L), 0,0, 66,10 },
	{ 2, -1, -1, (1<<8)+G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_24), 1,1, 63,1 },
	{ 3, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_25), 1,2, 63,1 },
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_26), 1,3, 63,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_27), 1,4, 63,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_28), 1,5, 63,1 },
	{ 7, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_29), 1,6, 63,1 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_30), 1,7, 63,1 },
	{ 0, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(gemfedit_string_31), 1,8, 63,1 },

/* FONT_PARAMS */

	{ -1, 1, 20, G_BOX, OF_NONE, OS_OUTLINED, C_UNION(0x21110L), 1,0, 54,16 },
	{ 2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_32), 20,1, 15,1 },
	{ 3, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[0]), 2,3, 51,1 }, /* FONT_NAME */
	{ 4, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[1]), 2,4, 24,1 }, /* FONT_ID */
	{ 5, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[2]), 2,5, 24,1 }, /* FONT_POINT */
	{ 6, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[3]), 29,5, 23,1 }, /* FONT_WIDTH */
	{ 7, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[4]), 29,6, 23,1 }, /* FONT_HEIGHT */
	{ 8, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[5]), 2,7, 22,1 }, /* FONT_TOP */
	{ 9, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[6]), 2,8, 22,1 }, /* FONT_ASCENT */
	{ 10, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_54), 29,8, 22,1 }, /* FONT_SYSTEM */
	{ 11, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[7]), 2,9, 22,1 }, /* FONT_HALF */
	{ 12, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_58), 29,9, 22,1 }, /* FONT_HORTABLE */
	{ 13, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[8]), 2,10, 22,1 }, /* FONT_DESCENT */
	{ 14, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_62), 29,10, 22,1 }, /* FONT_BIGENDIAN */
	{ 15, -1, -1, G_FTEXT, OF_EDITABLE, OS_NORMAL, C_UNION(&rs_tedinfo[9]), 2,11, 22,1 }, /* FONT_BOTTOM */
	{ 16, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_66), 29,11, 22,1 }, /* FONT_MONOSPACED */
	{ 17, -1, -1, G_BUTTON, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_67), 29,12, 22,1 }, /* FONT_COMPRESSED */
	{ 18, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[10]), 2,13, 22,1 }, /* FONT_FIRST_ADE */
	{ 19, -1, -1, G_FTEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[11]), 2,14, 22,1 }, /* FONT_LAST_ADE */
	{ 20, -1, -1, G_BUTTON, 0x5, OS_NORMAL, C_UNION(gemfedit_string_74), 33,14, 8,1 }, /* FONT_CANCEL */
	{ 0, -1, -1, G_BUTTON, 0x27, OS_NORMAL, C_UNION(gemfedit_string_75), 44,14, 8,1 }, /* FONT_OK */

/* PANEL */

	{ -1, 1, 1, G_BOX, OF_NONE, OS_NORMAL, C_UNION(0x74001100L), 0,0, 24,20 }, /* PANEL_BG */
	{ 0, 2, 17, G_BOX, OF_NONE, OS_OUTLINED, C_UNION(0x21100L), 2,1, 20,18 }, /* PANEL_BOX */
	{ 3, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_76), 2,1, 16,1 }, /* PANEL_FIRST */
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_77), 2,2, 16,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_78), 2,3, 16,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_79), 2,4, 16,1 },
	{ 7, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_80), 2,5, 16,1 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_81), 2,6, 16,1 },
	{ 9, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_82), 2,7, 16,1 },
	{ 10, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_83), 2,8, 16,1 },
	{ 11, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_84), 2,9, 16,1 },
	{ 12, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_85), 2,10, 16,1 },
	{ 13, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_86), 2,11, 16,1 },
	{ 14, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_87), 2,12, 16,1 },
	{ 15, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_88), 2,13, 16,1 },
	{ 16, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_89), 2,14, 16,1 },
	{ 17, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_90), 2,15, 16,1 },
	{ 1, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(gemfedit_string_91), 2,16, 16,1 }, /* PANEL_LAST */

/* ABOUT_DIALOG */

	{ -1, 1, 6, G_BOX, OF_FL3DBAK, OS_OUTLINED, C_UNION(0x21100L), 0,0, 32,13 },
	{ 2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_92), 3,1, 18,1 },
	{ 3, -1, -1, G_TEXT, OF_NONE, OS_NORMAL, C_UNION(&rs_tedinfo[12]), 3,2, 1040,1 },
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_96), 3,4, 24,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_97), 3,6, 11,1 }, /* ABOUT_VERSION */
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_98), 3,8, 11,1 }, /* ABOUT_DATE */
	{ 0, -1, -1, G_BUTTON, 0x627, OS_NORMAL, C_UNION(gemfedit_string_99), 22,11, 8,1 },

/* HELP_DIALOG */

	{ -1, 1, 8, G_BOX, OF_FL3DBAK, OS_OUTLINED, C_UNION(0x21100L), 0,0, 39,13 },
	{ 2, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_100), 3,1, 19,1 },
	{ 3, -1, -1, G_BUTTON, 0x607, OS_NORMAL, C_UNION(gemfedit_string_101), 29,11, 8,1 },
	{ 4, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_102), 3,3, 29,1 },
	{ 5, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_103), 3,4, 31,1 },
	{ 6, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_104), 3,5, 34,1 },
	{ 7, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_105), 3,6, 33,1 },
	{ 8, -1, -1, G_STRING, OF_NONE, OS_NORMAL, C_UNION(gemfedit_string_106), 3,8, 29,1 },
	{ 0, -1, -1, G_STRING, OF_LASTOB, OS_NORMAL, C_UNION(gemfedit_string_107), 3,9, 26,1 }
};


static OBJECT *rs_trindex[NUM_TREE] = {
	&rs_object[0], /* MAINMENU */
	&rs_object[30], /* CHARSET */
	&rs_object[39], /* FONT_PARAMS */
	&rs_object[60], /* PANEL */
	&rs_object[78], /* ABOUT_DIALOG */
	&rs_object[85] /* HELP_DIALOG */
};





#if RSC_STATIC_FILE

#if RSC_NAMED_FUNCTIONS
#ifdef __STDC__
_WORD gemfedit_rsc_load(void)
#else
_WORD gemfedit_rsc_load()
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
_WORD gemfedit_rsc_gaddr(_WORD type, _WORD idx, void *gaddr)
#else
_WORD gemfedit_rsc_gaddr(type, idx, gaddr)
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
_WORD gemfedit_rsc_free(void)
#else
_WORD gemfedit_rsc_free()
#endif
{
#if NUM_OBS != 0
	hrelease_objs(rs_object, NUM_OBS);
#endif
	return 1;
}

#endif /* RSC_NAMED_FUNCTIONS */

#else /* !RSC_STATIC_FILE */
int rs_numstrings = 131;
int rs_numfrstr = 23;

int rs_nuser = 0;
int rs_numimages = 0;
int rs_numbb = 0;
int rs_numfrimg = 0;
int rs_numib = 0;
int rs_numcib = 0;
int rs_numti = 13;
int rs_numobs = 94;
int rs_numtree = 6;

char rs_name[] = "gemfedit.rsc";

int _rsc_format = 2; /* RSC_FORM_SOURCE2 */
#endif /* RSC_STATIC_FILE */
