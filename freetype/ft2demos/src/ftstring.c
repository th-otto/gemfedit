/****************************************************************************/
/*                                                                          */
/*  The FreeType project -- a free and portable quality TrueType renderer.  */
/*                                                                          */
/*  Copyright 1996-2007, 2009-2016 by                                       */
/*  D. Turner, R.Wilhelm, and W. Lemberg                                    */
/*                                                                          */
/*                                                                          */
/*  ftstring.c - simple text string display                                 */
/*                                                                          */
/****************************************************************************/


#include "ftcommon.h"
#include "common.h"
#include "mlgetopt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <freetype/ftlcdfil.h>

#define CELLSTRING_HEIGHT  8
#define MAXPTSIZE          500			/* dtp */


static const char *Sample[] = {
	"The quick brown fox jumps over the lazy dog",

	/* Luís argüia à Júlia que «brações, fé, chá, óxido, pôr, zângão» */
	/* eram palavras do português */
	"Lu\303\255s arg\303\274ia \303\240 J\303\272lia que \302\253bra\303\247\303\265es, "
	"f\303\251, ch\303\241, \303\263xido, p\303\264r, z\303\242ng\303\243o\302\273 eram "
	"palavras do portugu\303\252s",

	/* Ο καλύμνιος σφουγγαράς ψιθύρισε πως θα βουτήξει χωρίς να διστάζει */
	"\316\237 \316\272\316\261\316\273\317\215\316\274\316\275\316\271\316\277\317\202 \317\203"
	"\317\206\316\277\317\205\316\263\316\263\316\261\317\201\316\254\317\202 \317\210\316\271"
	"\316\270\317\215\317\201\316\271\317\203\316\265 \317\200\317\211\317\202 \316\270\316\261 "
	"\316\262\316\277\317\205\317\204\316\256\316\276\316\265\316\271 \317\207\317\211\317\201"
	"\316\257\317\202 \316\275\316\261 \316\264\316\271\317\203\317\204\316\254\316\266\316\265\316\271",

	/* Съешь ещё этих мягких французских булок да выпей же чаю */
	"\320\241\321\212\320\265\321\210\321\214 \320\265\321\211\321\221 \321\215\321\202\320\270"
	"\321\205 \320\274\321\217\320\263\320\272\320\270\321\205 \321\204\321\200\320\260\320\275"
	"\321\206\321\203\320\267\321\201\320\272\320\270\321\205 \320\261\321\203\320\273\320\276"
	"\320\272 \320\264\320\260 \320\262\321\213\320\277\320\265\320\271 \320\266\320\265 \321\207\320\260\321\216",

	/* 天地玄黃，宇宙洪荒。日月盈昃，辰宿列張。寒來暑往，秋收冬藏。 */
	"\345\244\251\345\234\260\347\216\204\351\273\203\357\274\214\345\256\207\345\256\231\346\264\252\350\215\222\343\200\202\346\227\245"
	"\346\234\210\347\233\210\346\230\203\357\274\214\350\276\260\345\256\277\345\210\227\345\274\265\343\200\202\345\257\222\344\276\206"
	"\346\232\221\345\276\200\357\274\214\347\247\213\346\224\266\345\206\254\350\227\217\343\200\202",

	/* いろはにほへと ちりぬるを わかよたれそ つねならむ */
	/* うゐのおくやま けふこえて あさきゆめみし ゑひもせす */
	"\343\201\204\343\202\215\343\201\257\343\201\253\343\201\273\343\201\270\343\201\250 \343\201\241\343\202\212\343\201\254\343\202\213"
	"\343\202\222 \343\202\217\343\201\213\343\202\210\343\201\237\343\202\214\343\201\235 \343\201\244\343\201\255\343\201\252\343\202\211"
	"\343\202\200 \343\201\206\343\202\220\343\201\256\343\201\212\343\201\217\343\202\204\343\201\276 \343\201\221\343\201\265\343\201\223"
	"\343\201\210\343\201\246 \343\201\202\343\201\225\343\201\215\343\202\206\343\202\201\343\201\277\343\201\227 \343\202\221\343\201\262"
	"\343\202\202\343\201\233\343\201\231",

	/* 키스의 고유조건은 입술끼리 만나야 하고 특별한 기술은 필요치 않다 */
	"\355\202\244\354\212\244\354\235\230 \352\263\240\354\234\240\354\241\260\352\261\264\354\235\200 \354\236\205\354\210\240\353\201\274"
	"\353\246\254 \353\247\214\353\202\230\354\225\274 \355\225\230\352\263\240 \355\212\271\353\263\204\355\225\234 \352\270\260"
	"\354\210\240\354\235\200 \355\225\204\354\232\224\354\271\230 \354\225\212\353\213\244"
};

enum
{
	RENDER_MODE_STRING,
	RENDER_MODE_KERNCMP,
	N_RENDER_MODES
};

static struct status_
{
	int width;
	int height;

	int render_mode;
	unsigned long encoding;
	int res;
	int ptsize;							/* current point size */
	int angle;
	const char *text;

	FTDemo_String_Context sc;

	FT_Byte gamma_ramp[256];			/* for show only */
	FT_Matrix trans_matrix;
	int font_index;
	const char *header;
	char header_buffer[256];

} status =
{
	DIM_X, DIM_Y, RENDER_MODE_STRING, FT_ENCODING_UNICODE, 72, 48, 0, NULL,
	{ 0, 0, 0, 0, NULL },
	{ 0 },
	{ 0, 0, 0, 0 },
	0, NULL,
	{ 0 }
};

static FTDemo_Display *display;
static FTDemo_Handle *handle;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                   E V E N T   H A N D L I N G                   ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

static void event_help(void)
{
	char buf[256];
	char version[64];
	const char *format;
	FT_Int major, minor, patch;
	grEvent dummy_event;

	FT_Library_Version(handle->library, &major, &minor, &patch);

	format = patch ? "%d.%d.%d" : "%d.%d";
	sprintf(version, format, major, minor, patch);

	FTDemo_Display_Clear(display);
	grSetLineHeight(10);
	grGotoxy(0, 0);
	grSetMargin(2, 1);
	grGotobitmap(display->bitmap);

	sprintf(buf, "FreeType String Viewer - part of the FreeType %s test suite", version);

	grWriteln(buf);
	grLn();
	grWriteln("This program is used to display a string of text using");
	grWriteln("the new convenience API of the FreeType 2 library.");
	grLn();
	grWriteln("Use the following keys :");
	grLn();
	grWriteln("  F1 or ?   : display this help screen");
	grLn();
	grWriteln("  b         : toggle embedded bitmaps (and disable rotation)");
	grWriteln("  f         : toggle forced auto-hinting");
	grWriteln("  h         : toggle outline hinting");
	grLn();
	grWriteln("  1-2       : select rendering mode");
	grWriteln("  l         : cycle through anti-aliasing modes");
	grWriteln("  k         : cycle through kerning modes");
	grWriteln("  t         : cycle through kerning degrees");
	grWriteln("  Space     : cycle through color");
	grWriteln("  Tab       : cycle through sample strings");
	grWriteln("  V         : toggle vertical rendering");
	grLn();
	grWriteln("  g         : increase gamma by 0.1");
	grWriteln("  v         : decrease gamma by 0.1");
	grLn();
	grWriteln("  n         : next font");
	grWriteln("  p         : previous font");
	grLn();
	grWriteln("  Up        : increase pointsize by 1 unit");
	grWriteln("  Down      : decrease pointsize by 1 unit");
	grWriteln("  Page Up   : increase pointsize by 10 units");
	grWriteln("  Page Down : decrease pointsize by 10 units");
	grLn();
	grWriteln("  Right     : rotate counter-clockwise");
	grWriteln("  Left      : rotate clockwise");
	grWriteln("  F7        : big rotate counter-clockwise");
	grWriteln("  F8        : big rotate clockwise");
	grLn();
	grWriteln("press any key to exit this help screen");

	grRefreshSurface(display->surface);
	grListenSurface(display->surface, gr_event_key, &dummy_event);
}


static void event_font_change(int delta)
{
	if (status.font_index + delta >= handle->num_fonts || status.font_index + delta < 0)
		return;

	status.font_index += delta;

	FTDemo_Set_Current_Font(handle, handle->fonts[status.font_index]);
	FTDemo_Set_Current_Charsize(handle, status.ptsize, status.res);
	FTDemo_Update_Current_Flags(handle);

	FTDemo_String_Set(handle, status.text);
}


static void event_angle_change(int delta)
{
	double radian;
	FT_Fixed cosinus;
	FT_Fixed sinus;

	status.angle += delta;

	if (status.angle <= -180)
		status.angle += 360;
	if (status.angle > 180)
		status.angle -= 360;

	if (status.angle == 0)
	{
		status.sc.matrix = NULL;

		return;
	}

	status.sc.matrix = &status.trans_matrix;

	radian = status.angle * 3.14159265 / 180.0;
	cosinus = (FT_Fixed) (cos(radian) * 65536.0);
	sinus = (FT_Fixed) (sin(radian) * 65536.0);

	status.trans_matrix.xx = cosinus;
	status.trans_matrix.yx = sinus;
	status.trans_matrix.xy = -sinus;
	status.trans_matrix.yy = cosinus;
}


static void event_lcdmode_change(void)
{
	const char *lcd_mode;

	handle->lcd_mode++;

	switch (handle->lcd_mode)
	{
	case LCD_MODE_AA:
		lcd_mode = " normal AA";
		break;
	case LCD_MODE_LIGHT:
		lcd_mode = " light AA";
		break;
	case LCD_MODE_LIGHT_SUBPIXEL:
		lcd_mode = " light AA (subpixel positioning)";
		break;
	case LCD_MODE_RGB:
		lcd_mode = " LCD (horiz. RGB)";
		break;
	case LCD_MODE_BGR:
		lcd_mode = " LCD (horiz. BGR)";
		break;
	case LCD_MODE_VRGB:
		lcd_mode = " LCD (vert. RGB)";
		break;
	case LCD_MODE_VBGR:
		lcd_mode = " LCD (vert. BGR)";
		break;
	default:
		handle->lcd_mode = LCD_MODE_MONO;
		lcd_mode = " monochrome";
	}

	sprintf(status.header_buffer, "mode changed to %s", lcd_mode);
	status.header = status.header_buffer;
}


static void event_color_change(void)
{
	static int i = 0;
	unsigned char r = i & 4 ? 0xff : 0;
	unsigned char g = i & 2 ? 0xff : 0;
	unsigned char b = i & 1 ? 0xff : 0;

	display->back_color = grFindColor(display->bitmap, r, g, b, 0xff);
	display->fore_color = grFindColor(display->bitmap, ~r, ~g, ~b, 0xff);

	i++;
}


static void event_text_change(void)
{
	static int i = 0;

	status.text = Sample[i];

	i++;
	if (i >= (int) (sizeof(Sample) / sizeof(Sample[0])))
		i = 0;
}

static void event_gamma_change(double delta)
{
	int i;

	display->gamma += delta;

	if (display->gamma > 3.0)
		display->gamma = 3.0;
	else if (display->gamma < 0.1)
		display->gamma = 0.1;

	grSetGlyphGamma(display->gamma);

	for (i = 0; i < 256; i++)
		status.gamma_ramp[i] = (FT_Byte) (pow((double) i / 255., display->gamma) * 255. + 0.5);
}


static void event_size_change(int delta)
{
	status.ptsize += delta;

	if (status.ptsize < 1 * 64)
		status.ptsize = 1 * 64;
	else if (status.ptsize > MAXPTSIZE * 64)
		status.ptsize = MAXPTSIZE * 64;

	FTDemo_Set_Current_Charsize(handle, status.ptsize, status.res);
}


static void event_render_mode_change(int delta)
{
	if (delta)
	{
		status.render_mode = (status.render_mode + delta) % N_RENDER_MODES;

		if (status.render_mode < 0)
			status.render_mode += N_RENDER_MODES;
	}

	switch (status.render_mode)
	{
	case RENDER_MODE_STRING:
		status.header = NULL;
		break;

	case RENDER_MODE_KERNCMP:
		status.header = "Kerning comparison";
		break;
	}
}


static int Process_Event(grEvent *event)
{
	FTDemo_String_Context *sc = &status.sc;
	int ret = 0;

	if (event->key >= '1' && event->key < '1' + N_RENDER_MODES)
	{
		status.render_mode = event->key - '1';
		event_render_mode_change(0);

		return ret;
	}

	switch (event->key)
	{
	case grKeyEsc:
	case grKEY('q'):
		ret = 1;
		break;

	case grKeyF1:
	case grKEY('?'):
		event_help();
		break;

	case grKEY('b'):
		handle->use_sbits = !handle->use_sbits;
		status.header = handle->use_sbits
			? "embedded bitmaps are now used when available" : "embedded bitmaps are now ignored";

		FTDemo_Update_Current_Flags(handle);
		break;

	case grKEY('f'):
		handle->autohint = !handle->autohint;
		status.header = handle->autohint
			? "forced auto-hinting is now on" : "forced auto-hinting is now off";

		FTDemo_Update_Current_Flags(handle);
		break;

	case grKEY('h'):
		handle->hinted = !handle->hinted;
		status.header = handle->hinted
			? "glyph hinting is now active" : "glyph hinting is now ignored";

		FTDemo_Update_Current_Flags(handle);
		break;

	case grKEY('l'):
		event_lcdmode_change();

		FTDemo_Update_Current_Flags(handle);
		break;

	case grKEY('k'):
		sc->kerning_mode = (sc->kerning_mode + 1) % N_KERNING_MODES;
		status.header =
			sc->kerning_mode == KERNING_MODE_SMART
			? "pair kerning and side bearing correction is now active"
			: sc->kerning_mode == KERNING_MODE_NORMAL
			? "pair kerning is now active" : "pair kerning is now ignored";
		break;

	case grKEY('t'):
		sc->kerning_degree = (sc->kerning_degree + 1) % N_KERNING_DEGREES;
		status.header =
			sc->kerning_degree == KERNING_DEGREE_NONE
			? "no track kerning"
			: sc->kerning_degree == KERNING_DEGREE_LIGHT
			? "light track kerning active"
			: sc->kerning_degree == KERNING_DEGREE_MEDIUM
			? "medium track kerning active" : "tight track kerning active";
		break;

	case grKeySpace:
		event_color_change();
		break;

	case grKeyTab:
		event_text_change();
		FTDemo_String_Set(handle, status.text);
		break;

	case grKEY('V'):
		sc->vertical = !sc->vertical;
		status.header = sc->vertical ? "using vertical layout" : "using horizontal layout";
		break;

	case grKEY('g'):
		event_gamma_change(0.1);
		break;

	case grKEY('v'):
		event_gamma_change(-0.1);
		break;

	case grKEY('n'):
		event_font_change(1);
		break;

	case grKEY('p'):
		event_font_change(-1);
		break;

	case grKeyUp:
		event_size_change(64);
		break;
	case grKeyDown:
		event_size_change(-64);
		break;
	case grKeyPageUp:
		event_size_change(640);
		break;
	case grKeyPageDown:
		event_size_change(-640);
		break;

	case grKeyLeft:
		event_angle_change(-3);
		break;
	case grKeyRight:
		event_angle_change(3);
		break;
	case grKeyF7:
		event_angle_change(-30);
		break;
	case grKeyF8:
		event_angle_change(30);
		break;

	default:
		break;
	}

	return ret;
}


static void gamma_ramp_draw(FT_Byte gamma_ramp[256], grBitmap *bitmap)
{
	int i, x, y;
	int bpp = bitmap->pitch / bitmap->width;
	FT_Byte *p = (FT_Byte *) bitmap->buffer;

	if (bitmap->pitch < 0)
		p += -bitmap->pitch * (bitmap->rows - 1);

	x = (bitmap->width - 256) / 2;
	y = (bitmap->rows + 256) / 2;

	for (i = 0; i < 256; i++)
		p[bitmap->pitch * (y - i) + bpp * (x + gamma_ramp[i])] ^= 0xFF;
}


static void write_header(FT_Error error_code)
{
	FTDemo_Draw_Header(handle, display, status.ptsize, status.res, -1, error_code);

	if (status.header)
		grWriteCellString(display->bitmap, 0, 2 * HEADER_HEIGHT, status.header, display->fore_color);

	grRefreshSurface(display->surface);
}


static void usage(char *execname)
{
	fprintf(stderr,
			"\n"
			"ftstring: string viewer -- part of the FreeType project\n"
			"-------------------------------------------------------\n\n");
	fprintf(stderr, "Usage: %s [options] pt font ...\n\n", execname);
	fprintf(stderr,
			"  pt        The point size for the given resolution.\n"
			"            If resolution is 72dpi, this directly gives the\n"
			"            ppem value (pixels per EM).\n");
	fprintf(stderr,
			"  font      The font file(s) to display.\n"
			"            For Type 1 font files, ftstring also tries to attach\n"
			"            the corresponding metrics file (with extension\n"
			"            `.afm' or `.pfm').\n\n");
	fprintf(stderr,
			"  -w W      Set the window width to W pixels (default: %dpx).\n"
			"  -h H      Set the window height to H pixels (default: %dpx).\n\n", DIM_X, DIM_Y);
	fprintf(stderr,
			"  -r R      Use resolution R dpi (default: 72dpi).\n"
			"  -e enc    Specify encoding tag (default: no encoding).\n"
			"            Common values: `unic' (Unicode), `symb' (symbol),\n"
			"            `ADOB' (Adobe standard), `ADBC' (Adobe custom).\n"
			"  -m text   Use `text' for rendering.\n\n"
			"  -v        Show version.\n\n");

	exit(1);
}


static void parse_cmdline(int *argc, char ***argv)
{
	char *execname;
	int option;

	execname = ft_basename((*argv)[0]);

	while (1)
	{
		option = getopt(*argc, *argv, "e:h:m:r:vw:");

		if (option == -1)
			break;

		switch (option)
		{
		case 'e':
			status.encoding = FTDemo_Make_Encoding_Tag(optarg);
			break;

		case 'h':
			status.height = atoi(optarg);
			if (status.height < 1)
				usage(execname);
			break;

		case 'm':
			if (*argc < 3)
				usage(execname);
			status.text = optarg;
			break;

		case 'r':
			status.res = atoi(optarg);
			if (status.res < 1)
				usage(execname);
			break;

		case 'v':
			{
				FT_Int major, minor, patch;

				FT_Library_Version(handle->library, &major, &minor, &patch);

				printf("ftstring (FreeType) %d.%d", major, minor);
				if (patch)
					printf(".%d", patch);
				printf("\n");
				exit(0);
			}
			/* break; */

		case 'w':
			status.width = atoi(optarg);
			if (status.width < 1)
				usage(execname);
			break;

		default:
			usage(execname);
			break;
		}
	}

	*argc -= optind;
	*argv += optind;

	if (*argc <= 1)
		usage(execname);

	status.ptsize = (int) (atof(*argv[0]) * 64.0);
	if (status.ptsize == 0)
		status.ptsize = 64;

	(*argc)--;
	(*argv)++;
}


int main(int argc, char **argv)
{
	grEvent event;

	/* Initialize engine */
	handle = FTDemo_New();

	parse_cmdline(&argc, &argv);

	FT_Library_SetLcdFilter(handle->library, FT_LCD_FILTER_LIGHT);

	handle->encoding = status.encoding;
	handle->use_sbits = 0;
	FTDemo_Update_Current_Flags(handle);

	for (; argc > 0; argc--, argv++)
	{
		error = FTDemo_Install_Font(handle, argv[0], 0, 0);

		if (error)
		{
			fprintf(stderr, "failed to install %s", argv[0]);
			if (error == FT_Err_Invalid_CharMap_Handle)
				fprintf(stderr, ": missing valid charmap\n");
			else
				fprintf(stderr, "\n");
		}
	}

	if (handle->num_fonts == 0)
		PanicZ("could not open any font file");

	display = FTDemo_Display_New(gr_pixel_mode_rgb24, status.width, status.height);

	if (!display)
		PanicZ("could not allocate display surface");

	grSetTitle(display->surface, "FreeType String Viewer - press ? for help");

	status.header = NULL;

	if (!status.text)
		event_text_change();

	event_color_change();
	event_gamma_change(0);
	event_font_change(0);

	do
	{
		FTDemo_Display_Clear(display);

		gamma_ramp_draw(status.gamma_ramp, display->bitmap);

		switch (status.render_mode)
		{
		case RENDER_MODE_STRING:
			status.sc.center = 1L << 15;
			error = FTDemo_String_Draw(handle, display,
									   &status.sc, display->bitmap->width / 2, display->bitmap->rows / 2);
			break;

		case RENDER_MODE_KERNCMP:
			{
				FTDemo_String_Context sc = status.sc;
				FT_Int x, y;
				FT_Int height;

				x = 55;

				height = (status.ptsize * status.res / 72 + 32) >> 6;
				if (height < CELLSTRING_HEIGHT)
					height = CELLSTRING_HEIGHT;

				/* First line: none */
				sc.center = 0;
				sc.kerning_mode = 0;
				sc.kerning_degree = 0;
				sc.vertical = 0;
				sc.matrix = NULL;

				y = CELLSTRING_HEIGHT * 2 + display->bitmap->rows / 4 + height;
				grWriteCellString(display->bitmap, 5,
								  y - (height + CELLSTRING_HEIGHT) / 2, "none", display->fore_color);
				error = FTDemo_String_Draw(handle, display, &sc, x, y);

				/* Second line: track kern only */
				sc.kerning_degree = status.sc.kerning_degree;

				y += height;
				grWriteCellString(display->bitmap, 5,
								  y - (height + CELLSTRING_HEIGHT) / 2, "track", display->fore_color);
				error = FTDemo_String_Draw(handle, display, &sc, x, y);

				/* Third line: track kern + pair kern */
				sc.kerning_mode = status.sc.kerning_mode;

				y += height;
				grWriteCellString(display->bitmap, 5,
								  y - (height + CELLSTRING_HEIGHT) / 2, "both", display->fore_color);
				error = FTDemo_String_Draw(handle, display, &sc, x, y);
			}
			break;
		}

		write_header(error);

		status.header = 0;
		grListenSurface(display->surface, 0, &event);
	} while (!Process_Event(&event));

	printf("Execution completed successfully.\n");

	FTDemo_Display_Done(display);
	FTDemo_Done(handle);

	return 0;
}
