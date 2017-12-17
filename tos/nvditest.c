#include <portaes.h>
#include <portvdi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <osbind.h>
#include <mint/arch/nf_ops.h>

#define MAIN_WKIND (NAME | CLOSER | MOVER)

static _WORD app_id = -1;
static _WORD aeshandle;
static _WORD vdihandle = 0;
static _WORD gl_wchar, gl_hchar;
static _WORD work_in[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
static _WORD dummy;
static _WORD mainwin;
static _BOOL quit_app;

_WORD workout[57];
_WORD xworkout[57];

unsigned short const atari_to_unicode[256] = {
/* 00 */	0x0000, 0x25b2, 0x25bc, 0x25ba, 0x25c4, 0x25aa, 0x2611, 0x2612,
/* 08 */	0x221a, 0x231a, 0x266b, 0x266a, 0x2191, 0x2193, 0x2192, 0x2190,
/* 10 */	0x24ea, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466,
/* 18 */	0x2467, 0x2468, 0x0259, 0x241b, 0x26f2, 0x26f3, 0x26f4, 0x26f5,
/* 20 */	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
/* 28 */	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
/* 30 */	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
/* 38 */	0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
/* 40 */	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
/* 48 */	0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
/* 50 */	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
/* 58 */	0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
/* 60 */	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
/* 68 */	0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
/* 70 */	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
/* 78 */	0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2206,
/* 80 */	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
/* 88 */	0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
/* 90 */	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
/* 98 */	0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x00df, 0x0192,
/* a0 */	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
/* a8 */	0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
/* b0 */	0x00e3, 0x00f5, 0x00d8, 0x00f8, 0x0153, 0x0152, 0x00c0, 0x00c3,
/* b8 */	0x00d5, 0x00a8, 0x00b4, 0x2020, 0x00b6, 0x00a9, 0x00ae, 0x2122,
/* c0 */	0x0133, 0x0132, 0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5,
/* c8 */	0x05d6, 0x05d7, 0x05d8, 0x05d9, 0x05db, 0x05dc, 0x05de, 0x05e0,
/* d0 */	0x05e1, 0x05e2, 0x05e4, 0x05e6, 0x05e7, 0x05e8, 0x05e9, 0x05ea,
/* d8 */	0x05df, 0x05da, 0x05dd, 0x05e3, 0x05e5, 0x00a7, 0x2227, 0x221e,
/* e0 */	0x03b1, 0x03b2, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
/* e8 */	0x03a6, 0x0398, 0x03a9, 0x03b4, 0x222e, 0x03c6, 0x2208, 0x2229,
/* f0 */	0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
/* f8 */	0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x00b3, 0x00af
};

static void destroy_win(void)
{
	if (mainwin > 0)
	{
		wind_close(mainwin);
		wind_delete(mainwin);
		mainwin = -1;
	}
	if (vdihandle > 0)
	{
		vst_unload_fonts(vdihandle, 0);
		v_clsvwk(vdihandle);
		vdihandle = 0;
	}
}


static _BOOL create_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	_WORD width, height;
	
	width = 400;
	height = 300;
	
	/* create a window */
	gr.g_x = 100;
	gr.g_y = 100;
	gr.g_w = width;
	gr.g_h = height;
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	mainwin = wind_create_grect(wkind, &desk);
	if (mainwin < 0)
	{
		form_alert(1, "[1][Cannot create window][OK]");
		return FALSE;
	}

	wind_set_str(mainwin, WF_NAME, "");
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	wind_open_grect(mainwin, &gr);
	
	return TRUE;
}


static void mainwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[8];
	_WORD font_cw;
	_WORD font_ch, font_ch2;
	_WORD xoff;
	int x, y;
	
	v_hide_c(vdihandle);
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	wind_get_grect(mainwin, WF_FIRSTXYWH, &gr);
	while (gr.g_w > 0 && gr.g_h > 0)
	{
		if (rc_intersect(area, &gr))
		{
			pxy[0] = gr.g_x;
			pxy[1] = gr.g_y;
			pxy[2] = gr.g_x + gr.g_w - 1;
			pxy[3] = gr.g_y + gr.g_h - 1;
			vs_clip(vdihandle, 1, pxy);
			vswr_mode(vdihandle, MD_REPLACE);
			vsf_color(vdihandle, G_WHITE);
			vsf_perimeter(vdihandle, 0);
			vsf_interior(vdihandle, FIS_SOLID);
			vr_recfl(vdihandle, pxy);
			vst_font(vdihandle, 1);
			vst_alignment(vdihandle, ALI_LEFT, ALI_TOP, &dummy, &dummy);
			vst_color(vdihandle, G_BLACK);
			vst_point(vdihandle, 10, &dummy, &dummy, &font_cw, &font_ch);
			xoff = 18 * font_cw;

#undef FONT_ID
#define FONT_ID 15040 /* Arial fVDI */
#undef FONT_ID
#define FONT_ID 1 /* System font */
#undef FONT_ID
#define FONT_ID 9908 /* Arial */
#undef FONT_ID
#define FONT_ID 5003 /* Swiss 721 */
#undef FONT_ID
#define FONT_ID 5031 /* Baskerville */

			vst_font(vdihandle, FONT_ID);
			vst_point(vdihandle, 10, &dummy, &dummy, &dummy, &font_ch2);
			if (font_ch2 > font_ch)
				font_ch = font_ch2;
			vst_font(vdihandle, 1);

			v_gtext(vdihandle, work.g_x, work.g_y + font_ch *  0, "Hello, world");
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					char c = y * 16 + x;
					v_gtextn(vdihandle, work.g_x + x * font_cw, work.g_y + font_ch * (y + 1), &c, 1);
				}
			}
			
			vst_font(vdihandle, FONT_ID);
			vst_alignment(vdihandle, ALI_LEFT, ALI_TOP, &dummy, &dummy);
			vst_color(vdihandle, G_BLACK);
			vst_point(vdihandle, 10, &dummy, &dummy, &dummy, &dummy);
			font_cw = 8;

#define USE_UNICODE 1
#if USE_UNICODE
			vst_map_mode(vdihandle, 2);
#else
			vst_map_mode(vdihandle, 1);
#endif
			v_gtext(vdihandle, work.g_x + xoff, work.g_y + font_ch *  0, "Hello, world");
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					char c = y * 16 + x;
#if USE_UNICODE
					unsigned short wc = atari_to_unicode[c];
					v_gtext16n(vdihandle, work.g_x + xoff + (x * 2) * font_cw, work.g_y + font_ch * (y + 1), &wc, 1);
#else
					v_gtextn(vdihandle, work.g_x + xoff + (x * 2) * font_cw, work.g_y + font_ch * (y + 1), &c, 1);
#endif
				}
			}
			
#if 1
			{
				unsigned short unicode;
				unsigned short i, x;
				_WORD minade, maxade;
				_WORD distances[5];
				_WORD max_width[1];
				_WORD effects[3];
				
				vst_map_mode(vdihandle, 1);
				minade = maxade = 0;
				vqt_fontinfo(vdihandle, &minade, &maxade, distances, max_width, effects);
				nf_debugprintf("mapping 1 -> 2 (%d-%d)\n", minade, maxade);
				for (i = minade, x = 0; i <= maxade; i++)
				{
					unicode = vqt_char_index(vdihandle, i, 1, 2);
					if ((x % 8) == 0)
						nf_debugprintf("/* %02x */\t", i);
					nf_debugprintf(" 0x%04x,", unicode);
					x++;
					if ((x % 8) == 0)
						nf_debugprintf("\n");
				}
				if ((x % 8) != 0)
					nf_debugprintf("\n");
			}
#endif
#if 1
			{
				unsigned short unicode;
				unsigned short i, x;
				_WORD minade, maxade;
				_WORD distances[5];
				_WORD max_width[1];
				_WORD effects[3];
				_WORD mode;
				
				mode = vst_map_mode(vdihandle, 0);
				minade = maxade = 0;
				vqt_fontinfo(vdihandle, &minade, &maxade, distances, max_width, effects);
				nf_debugprintf("mapping 0 -> 2 (%d-%d), %d\n", minade, maxade, mode);
				for (i = minade, x = 0; i <= maxade; i++)
				{
					unicode = vqt_char_index(vdihandle, i, 0, 2);
					if ((x % 8) == 0)
						nf_debugprintf("/* %02x */\t", i);
					nf_debugprintf(" 0x%04x,", unicode);
					x++;
					if ((x % 8) == 0)
						nf_debugprintf("\n");
				}
				if ((x % 8) != 0)
					nf_debugprintf("\n");
			}
#endif
			vst_map_mode(vdihandle, 1);
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}


static void list_fonts(void)
{
	_WORD d;
	_WORD i;
	_WORD id;
	_WORD vector;
	char name[100];
	_WORD loaded_fonts;
	XFNT_INFO info;
	
	loaded_fonts = vst_load_fonts(vdihandle, 0);
	nf_debugprintf("fonts: %d + %d\n", workout[10], loaded_fonts);
	nf_debugprintf("vqt_name:\n");
	for (d = 1; d <= workout[10] + loaded_fonts; d++)
	{
		id = vqt_name(vdihandle, d, name);
		vector = name[32];
		name[32] = 0;
		nf_debugprintf("%d: %d %d %s\n", d, id, vector, name);
	}
	nf_debugprintf("vqt_xfntinfo:\n");
	for (d = 1; d <= workout[10] + loaded_fonts; d++)
	{
		info.pt_cnt = 0;
		info.id = 0;
		info.size = sizeof(info);
		vqt_xfntinfo(vdihandle, 0x0309, 0, d, &info);
		if (info.id)
		{
			nf_debugprintf("%d: %d %d %s %s: ", d, info.id, info.format, info.font_name, info.file_name1);
			for (i = 0; i < info.pt_cnt; i++)
				nf_debugprintf(" %u", info.pt_sizes[i]);
			nf_debugprintf("\n");
		}
	}
}


int main(void)
{
	_WORD event;
	_WORD message[8];
	_WORD k, kstate, mox, moy;
	
	app_id = appl_init();
	if (app_id < 0)
		return 1;
	
	aeshandle = graf_handle(&gl_wchar, &gl_hchar, &dummy, &dummy);

	vdihandle = aeshandle;
	(void) v_opnvwk(work_in, &vdihandle, workout);	/* VDI workstation needed */
	list_fonts();
	
	nf_debugprintf("mapmode(0): %d\n", vst_map_mode(vdihandle, 0));
	nf_debugprintf("mapmode(1): %d\n", vst_map_mode(vdihandle, 1));
	nf_debugprintf("mapmode(2): %d\n", vst_map_mode(vdihandle, 2));
	nf_debugprintf("mapmode(2): %d\n", vst_map_mode(vdihandle, 2));
	nf_debugprintf("mapmode(3): %d\n", vst_map_mode(vdihandle, 3));
	vst_map_mode(vdihandle, 1);

	create_window();
	
	graf_mouse(ARROW, NULL);
	
	while (!quit_app)
	{
		event = evnt_multi(MU_KEYBD | MU_MESAG | MU_BUTTON,
			2, 1, 1,
			0, 0, 0, 0, 0,
			0, 0, 0 ,0, 0,
			message,
			0L,
			&mox, &moy, &dummy, &kstate, &k, &dummy);
		
		if (event & MU_MESAG)
		{
			wind_update(BEG_UPDATE);
			switch (message[0])
			{
			case WM_CLOSED:
				quit_app = TRUE;
				break;

			case AP_TERM:
				quit_app = TRUE;
				break;

			case WM_SIZED:
				if (message[3] == mainwin)
				{
					GRECT gr;
					
					wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
					wind_get_grect(message[3], WF_WORKXYWH, &gr);
				}
				break;

			case WM_MOVED:
				if (message[3] == mainwin)
				{
					wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
				}
				break;

			case WM_TOPPED:
				if (message[3] == mainwin)
					wind_set_int(message[3], WF_TOP, 0);
				break;

			case WM_REDRAW:
				if (message[3] == mainwin)
					mainwin_draw((const GRECT *)&message[4]);
				break;

			case MN_SELECTED:
				break;
			}
			wind_update(END_UPDATE);
		}

		if (event & MU_KEYBD)
		{
			switch (k & 0xff)
			{
			case 0x0f:
				break;
			case 0x09:
				break;
			case 0x11:
				quit_app = TRUE;
				break;
			default:
				break;
			}
		}
	}
			
	destroy_win();
	
	appl_exit();
	return 0;
}
