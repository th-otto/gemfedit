#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __PUREC__
#include <portab.h>
#include <mint/arch/nf_ops.h>
#else
#define nf_debugprintf(s...)
#define _BOOL int
#define _WORD short
#define _UWORD unsigned short
#endif
#include "gemfedit.h"
static _WORD gl_wchar, gl_hchar;
#define GetTextSize(w, h) *(w) = gl_wchar, *(h) = gl_hchar
#define hfix_objs(a, b, c)
#define hrelease_objs(a, b)
#include "s_endian.h"
#include "fonthdr.h"
#include "version.h"

#if defined(__GEMLIB_MAJOR__) && (((__GEMLIB_MAJOR__) * 1000L + __GEMLIB_MINOR__) <= (0 * 1000L + 44))
# define wind_set_int(h, a, b) wind_set(h, a, b, 0, 0, 0)
# define form_dial_grect(a, in, out) form_dial(a, (in)->g_x, (in)->g_y, (in)->g_w, (in)->g_h, (out)->g_x, (out)->g_y, (out)->g_w, (out)->g_h)
#endif

#undef SWAP_W
#undef SWAP_L
#define SWAP_W(s) s = cpu_swab16(s)
#define SWAP_L(s) s = cpu_swab32(s)

#ifndef _BOOL
# define _BOOL int
#endif


typedef struct
{
    FONT_HDR *font[4];
} FONTS;


#define PANEL_Y_MARGIN 20
#define PANEL_X_MARGIN 20
#define PANEL_WKIND (NAME | CLOSER | MOVER)

#define MAIN_Y_MARGIN 10
#define MAIN_X_MARGIN 10
#define MAIN_WKIND (NAME | CLOSER | MOVER)

static _WORD app_id = -1;
static _WORD mainwin = -1;
static _WORD panelwin = -1;
static _WORD aeshandle;
static _WORD vdihandle = 0;
static _BOOL quit_app;
static OBJECT *menu;
static _WORD scalex = 16;
static _WORD scaley = 16;
static const int scaled_margin = 1;
static _WORD workout[57];
static _WORD xworkout[57];

#define F_NO_CHAR 0xffffu
typedef unsigned short fchar_t;

static _WORD font_cw;
static _WORD font_ch;
static _WORD last_display_cw;
static FONT_HDR fonthdr;
static unsigned char *fontmem;
static unsigned char *dat_table;
static uint16_t *off_table;
static int8_t *hor_table;
static char fontname[VDI_FONTNAMESIZE + 1];
static const char *fontfilename;
static const char *fontbasename;
static fchar_t numoffs;
static fchar_t cur_char;
static _BOOL font_changed = FALSE;
static MFDB screen_fdb;

#define FONT_BIG ((fonthdr.flags & FONTF_BIGENDIAN) != 0)

#if 0
LINEA *Linea;
VDIESC *Vdiesc;
LINEA_FUNP *Linea_funp;
#endif
FONTS *Fonts;

static char const program_name[] = "gemfedit";

#define is_font_loaded() (dat_table != NULL)

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static OBJECT *rs_tree(_WORD num)
{
	OBJECT *tree = NULL;
	rsrc_gaddr(R_TREE, num, &tree);
	return tree;
}

/* -------------------------------------------------------------------------- */
	
static char *rs_str(_WORD num)
{
	char *str = NULL;
	rsrc_gaddr(R_STRING, num, &str);
	return str;
}

/* -------------------------------------------------------------------------- */
	
static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen);
	dst[maxlen - 1] = '\0';
	len = strlen(dst);
	while (len > 0 && dst[len - 1] == ' ')
		dst[--len] = '\0';
}

/* -------------------------------------------------------------------------- */

static char *xbasename(const char *path)
{
	char *p = strrchr(path, '\\');
	char *q = strrchr(path, '/');
	if (p == NULL || q > p)
		p = q;
	if (p == NULL)
		p = (char *)path;
	else
		++p;
	return p;
}

/* -------------------------------------------------------------------------- */

static void cleanup(void)
{
	if (app_id >= 0)
	{
		menu_bar(menu, FALSE);
		rsrc_free();
		appl_exit();
		app_id = -1;
	}
}

/* -------------------------------------------------------------------------- */

static void destroy_win(void)
{
	if (mainwin > 0)
	{
		wind_close(mainwin);
		wind_delete(mainwin);
		mainwin = -1;
	}
	if (panelwin > 0)
	{
		wind_close(panelwin);
		wind_delete(panelwin);
		panelwin = -1;
	}
	if (vdihandle > 0)
	{
		v_clsvwk(vdihandle);
		vdihandle = 0;
	}
}

/* -------------------------------------------------------------------------- */

static void redraw_win(_WORD win)
{
	_WORD message[8];
	
	message[0] = WM_REDRAW;
	message[1] = gl_apid;
	message[2] = 0;
	message[3] = win;
	wind_get_grect(win, WF_WORKXYWH, (GRECT *)&message[4]);
	appl_write(gl_apid, 16, message);
}

/* -------------------------------------------------------------------------- */

static void set_panel_pos(void)
{
	GRECT gr, size;
	_WORD x, y, ox, oy;
	OBJECT *panel = rs_tree(PANEL);
	
	wind_get_grect(panelwin, WF_WORKXYWH, &gr);
	form_center_grect(panel, &size);
	objc_offset(panel, ROOT, &x, &y);
	ox = x - size.g_x;
	oy = y - size.g_y;
	panel[ROOT].ob_x = gr.g_x + ox;
	panel[ROOT].ob_y = gr.g_y + oy;
}

/* -------------------------------------------------------------------------- */

static void redraw_pixel(_WORD x, _WORD y)
{
	_WORD message[8];
	GRECT gr;
	OBJECT *panel;
	
	message[0] = WM_REDRAW;
	message[1] = gl_apid;
	message[2] = 0;
	message[3] = mainwin;
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	message[4] = gr.g_x + MAIN_X_MARGIN + scaled_margin + x * (scalex + scaled_margin);
	message[5] = gr.g_y + MAIN_Y_MARGIN + scaled_margin + y * (scaley + scaled_margin);
	message[6] = scalex;
	message[7] = scaley;
	appl_write(gl_apid, 16, message);
	message[3] = panelwin;
	set_panel_pos();
	wind_get_grect(panelwin, WF_WORKXYWH, &gr);
	panel = rs_tree(PANEL);
	objc_offset(panel, PANEL_FIRST + cur_char / 16, &message[4], &message[5]);
	message[4] += (cur_char % 16) * font_cw;
	message[6] = font_cw;
	message[7] = font_ch;
	appl_write(gl_apid, 16, message);
}

/* -------------------------------------------------------------------------- */

#define ror(x) (((x) >> 1) | ((x) & 1 ? 0x80 : 0))

static _BOOL char_testbit(fchar_t c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (!is_font_loaded())
		return FALSE;
	if (c < fonthdr.first_ade || c > fonthdr.last_ade)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (size_t)y * fonthdr.form_width + (o >> 3);
	return (*dat & mask) != 0;
}

/* -------------------------------------------------------------------------- */

static _BOOL char_togglebit(fchar_t c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (!is_font_loaded())
		return FALSE;
	if (c < fonthdr.first_ade || c > fonthdr.last_ade)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (size_t)y * fonthdr.form_width + (o >> 3);
	*dat ^= mask;
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL char_setbit(fchar_t c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (!is_font_loaded())
		return FALSE;
	if (c < fonthdr.first_ade || c > fonthdr.last_ade)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (size_t)y * fonthdr.form_width + (o >> 3);
	if (!(*dat & mask))
	{
		*dat |= mask;
		return TRUE;
	}
	return FALSE;
}

/* -------------------------------------------------------------------------- */

static _BOOL char_clearbit(fchar_t c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;
	
	if (!is_font_loaded())
		return FALSE;
	if (c < fonthdr.first_ade || c > fonthdr.last_ade)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (size_t)y * fonthdr.form_width + (o >> 3);
	if (*dat & mask)
	{
		*dat &= ~mask;
		return TRUE;
	}
	return FALSE;
}

/* -------------------------------------------------------------------------- */

static void draw_char(fchar_t c, _WORD x0, _WORD y0)
{
	_WORD x, y;
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat, *p;
	
	if (!is_font_loaded())
		return;
	if (c < fonthdr.first_ade || c > fonthdr.last_ade)
		return;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;

	vsf_color(vdihandle, G_BLACK);
	o = off_table[c];
	b = o & 0x7;
	dat = dat_table + (o >> 3);
	for (y = 0; y < height; y++, dat += fonthdr.form_width)
	{
		mask = 0x80 >> b;
		p = dat;
		for (x = 0; x < width; x++)
		{
			if (*p & mask)
			{
				_WORD pxy[4];
				
				pxy[0] = x0 + x * (scalex + scaled_margin);
				pxy[1] = y0 + y * (scaley + scaled_margin);
				pxy[2] = pxy[0] + scalex - 1;
				pxy[3] = pxy[1] + scaley - 1;
				vr_recfl(vdihandle, pxy);
			}
			mask = ror(mask);
			if (mask == 0x80)
				p++;
		}
	}
}

/* -------------------------------------------------------------------------- */

static void mainwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[8];
	_WORD x, y;
	_WORD scaled_w, scaled_h;
	char buf[80];
	_WORD cw;
	
	if (is_font_loaded() && cur_char >= fonthdr.first_ade && cur_char <= fonthdr.last_ade)
	{
		cw = off_table[cur_char - fonthdr.first_ade + 1] - off_table[cur_char - fonthdr.first_ade];
	} else
	{
		cw = font_cw;
	}
	v_hide_c(vdihandle);
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	scaled_w = cw * scalex + (cw + 1) * scaled_margin;
	scaled_h = font_ch * scaley + (font_ch + 1) * scaled_margin;
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
	
			vsf_color(vdihandle, G_RED);
			/*
			 * draw horizontal grid lines
			 */
			for (y = 0; y < (font_ch + 1); y++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN;
				pxy[1] = work.g_y + MAIN_Y_MARGIN + y * (scaley + scaled_margin);
				pxy[2] = pxy[0] + scaled_w - 1;
				pxy[3] = pxy[1] + scaled_margin - 1;
				vr_recfl(vdihandle, pxy);
			}
			/*
			 * draw vertical grid lines
			 */
			for (x = 0; x < (cw + 1); x++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN + x * (scalex + scaled_margin);
				pxy[1] = work.g_y + MAIN_Y_MARGIN;
				pxy[2] = pxy[0] + scaled_margin - 1;
				pxy[3] = pxy[1] + scaled_h - 1;
				vr_recfl(vdihandle, pxy);
			}
			
			x = work.g_x + MAIN_X_MARGIN + cw * (scalex + scaled_margin) + gl_wchar;
			y = work.g_y + MAIN_Y_MARGIN;
			if (cur_char >= fonthdr.first_ade && cur_char <= fonthdr.last_ade)
			{
				draw_char(cur_char, work.g_x + MAIN_X_MARGIN + scaled_margin, work.g_y + MAIN_Y_MARGIN + scaled_margin);
				sprintf(buf, "Char: $%02x", cur_char);
				v_gtext(vdihandle, x, y, buf);
			} else
			{
				v_gtext(vdihandle, x, y, "Char: -");
			}
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}

/* -------------------------------------------------------------------------- */

static void panelwin_draw(const GRECT *area)
{
	GRECT gr;
	OBJECT *panel = rs_tree(PANEL);
	
	set_panel_pos();
	v_hide_c(vdihandle);
	wind_get_grect(panelwin, WF_FIRSTXYWH, &gr);
	while (gr.g_w > 0 && gr.g_h > 0)
	{
		if (rc_intersect(area, &gr))
		{
			objc_draw_grect(panel, ROOT, MAX_DEPTH, &gr);
		}
		wind_get_grect(panelwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}

/* -------------------------------------------------------------------------- */

static void do_resize_window(_WORD cw)
{
	GRECT gr, desk;
	_WORD wkind;
	_WORD width, height;
	
	width = MAIN_X_MARGIN * 2 + scalex * cw + (cw + 1) * scaled_margin + 12 * gl_wchar;
	height = MAIN_Y_MARGIN * 2 + scaley * font_ch + (font_ch + 1) * scaled_margin;
	if (height < 5 * gl_hchar)
		height = 5 * gl_hchar;
	wind_get_grect(mainwin, WF_CURRXYWH, &gr);
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	gr.g_w = width;
	gr.g_h = height;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	wind_set_grect(mainwin, WF_CURRXYWH, &gr);
}

/* -------------------------------------------------------------------------- */

static void resize_window(void)
{
	do_resize_window(last_display_cw);
}

/* -------------------------------------------------------------------------- */

static void maybe_resize_window(void)
{
	_WORD cw;
	
	if (is_font_loaded() && cur_char >= fonthdr.first_ade && cur_char <= fonthdr.last_ade)
	{
		cw = off_table[cur_char - fonthdr.first_ade + 1] - off_table[cur_char - fonthdr.first_ade];
	} else
	{
		cw = font_cw;
	}
	if (cw != last_display_cw)
	{
		last_display_cw = cw;
		resize_window();
	}
}

/* -------------------------------------------------------------------------- */

static void panel_click(_WORD x, _WORD y)
{
	OBJECT *panel = rs_tree(PANEL);
	_WORD obj;
	_WORD ox, oy;
	
	set_panel_pos();
	obj = objc_find(panel, ROOT, MAX_DEPTH, x, y);
	if (obj >= PANEL_FIRST && obj <= PANEL_LAST && font_cw > 0)
	{
		objc_offset(panel, obj, &ox, &oy);
		cur_char = (obj - PANEL_FIRST) * 16 + (x - ox) / font_cw;
		maybe_resize_window();
		redraw_win(mainwin);
	}
}

/* -------------------------------------------------------------------------- */

static void handle_message(_WORD *message, _WORD mox, _WORD moy);

static void mainwin_click(_WORD x, _WORD y)
{
	GRECT gr;
	_WORD last_x = -1, last_y = -1;
	_WORD button, dummy;
	_WORD message[8];
	_WORD event;
	_WORD mox, moy;
	_WORD mode, nextmode;
	_BOOL changed;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	
	graf_mkstate(&mox, &moy, &button, &dummy);
	if (button & 2)
	{
		mode = 0;
		nextmode = 0;
	} else
	{
		mode = 2;
		x = (mox - gr.g_x - MAIN_X_MARGIN - scaled_margin) / (scalex + scaled_margin);
		y = (moy - gr.g_y - MAIN_Y_MARGIN - scaled_margin) / (scaley + scaled_margin);
		if (char_testbit(cur_char, x, y))
			nextmode = 0;
		else
			nextmode = 1;
	}
	do
	{
		x = (mox - gr.g_x - MAIN_X_MARGIN - scaled_margin) / (scalex + scaled_margin);
		y = (moy - gr.g_y - MAIN_Y_MARGIN - scaled_margin) / (scaley + scaled_margin);
		if (x != last_x || y != last_y)
		{
			switch (mode)
			{
			default:
			case 0:
				changed = char_clearbit(cur_char, x, y);
				break;
			case 1:
				changed = char_setbit(cur_char, x, y);
				break;
			case 2:
				changed = char_togglebit(cur_char, x, y);
				break;
			}
			if (changed)
			{
				redraw_pixel(x, y);
				font_changed = TRUE;
			}
			last_x = x;
			last_y = y;
			mode = nextmode;
		}
		event = evnt_multi(MU_MESAG | MU_BUTTON | MU_M1,
			256|1, 3, 3,
			1, mox, moy, 1, 1,
			0, 0, 0 ,0, 0,
			message,
			0L,
			&x, &y, &button, &dummy, &dummy, &dummy);
		if (event & MU_MESAG)
			handle_message(message, x, y);
		graf_mkstate(&mox, &moy, &button, &dummy);
	} while (button & 3);
}

/* -------------------------------------------------------------------------- */

static _BOOL open_screen(void)
{
	_WORD work_in[11];
	_WORD dummy;
	_WORD i;
	
	vdihandle = aeshandle;
	for (i = 0; i < 10; i++)
		work_in[i] = 1;
	work_in[10] = 2;
	v_opnvwk(work_in, &vdihandle, workout);	/* VDI workstation needed */
	screen_fdb.fd_addr = 0;
	screen_fdb.fd_w = workout[0] + 1;
	screen_fdb.fd_h = workout[1] + 1;
	screen_fdb.fd_wdwidth = screen_fdb.fd_w >> 4;
	vq_extnd(vdihandle, 1, xworkout);
	screen_fdb.fd_nplanes = xworkout[4];
	screen_fdb.fd_stand = FALSE;
	screen_fdb.fd_r1 = 0;
	screen_fdb.fd_r2 = 0;
	screen_fdb.fd_r3 = 0;
	
	/*
	 * enforce some default values
	 */
	vst_color(vdihandle, G_BLACK);
	vst_alignment(vdihandle, TA_LEFT, TA_TOP, &dummy, &dummy);
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL create_panel_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	OBJECT *panel = rs_tree(PANEL);
	
	/* create a window */
	form_center_grect(panel, &gr);
	gr.g_x = 100;
	gr.g_y = 100;
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = PANEL_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	gr.g_x = 20;
	gr.g_y = 20;
	rc_intersect(&desk, &gr);
	panelwin = wind_create_grect(wkind, &desk);
	if (panelwin < 0)
	{
		form_alert(1, rs_str(AL_NOWINDOW));
		return FALSE;
	}

	wind_set_str(panelwin, WF_NAME, "System Font");
	wind_open_grect(panelwin, &gr);
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL create_window(void)
{
	GRECT gr, desk, panel;
	_WORD wkind;
	_WORD width, height;
	
	width = MAIN_X_MARGIN * 2 + scalex * font_cw + (font_cw + 1) * scaled_margin;
	height = MAIN_Y_MARGIN * 2 + scaley * font_ch + (font_ch + 1) * scaled_margin;
	
	/* create a window */
	gr.g_x = 100;
	gr.g_y = 100;
	gr.g_w = width;
	gr.g_h = height;
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	wind_get_grect(panelwin, WF_CURRXYWH, &panel);
	gr.g_x = panel.g_x + panel.g_w + 20;
	gr.g_y = panel.g_y;
	rc_intersect(&desk, &gr);
	mainwin = wind_create_grect(wkind, &desk);
	if (mainwin < 0)
	{
		form_alert(1, rs_str(AL_NOWINDOW));
		return FALSE;
	}

	wind_set_str(mainwin, WF_NAME, "");
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	wind_open_grect(mainwin, &gr);
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

#ifdef __PUREC__
/*
 * the original Pure-C pcgemlib uses static arrays,
 * and there is no need to use the udef_* functions
 */
#define udef_vqt_attributes vqt_attributes
#define udef_vqf_attributes vqf_attributes
#define udef_vqm_attributes vqm_attributes
#define udef_vswr_mode vswr_mode
#define udef_vsm_color vsm_color
#define udef_vsm_type vsm_type
#define udef_vsm_height vsm_height
#define udef_vsf_color vsf_color
#define udef_vsf_interior vsf_interior
#define udef_vsf_perimeter vsf_perimeter
#define udef_vsf_style vsf_style 
#define udef_vr_recfl vr_recfl
#define udef_v_pmarker v_pmarker
#define udef_vst_color vst_color
#define udef_vst_rotation vst_rotation
#define udef_vst_alignment vst_alignment
#define udef_vst_height vst_height
#define udef_vrt_cpyfm vrt_cpyfm
#define udef_vro_cpyfm vro_cpyfm
#endif

/*
 * draw one line of characters in the panel window.
 * called by a user-defined object
 */
static _WORD __CDECL draw_font(PARMBLK *pb)
{
	_WORD tattrib[10];
	_WORD fattrib[5];
	_WORD mattrib[5];
	fchar_t c;
	_WORD dummy;
	fchar_t basec, ch;
	_WORD width;
	_WORD o;
	MFDB src;
	_WORD pxy[8];
	_WORD colors[2];
	_BOOL can_use_vrocpy;
	
	udef_vqt_attributes(aeshandle, tattrib);
	udef_vqf_attributes(aeshandle, fattrib);
	udef_vqm_attributes(aeshandle, mattrib);
	udef_vsf_color(aeshandle, G_WHITE);
	udef_vsf_interior(aeshandle, FIS_SOLID);
	udef_vsf_perimeter(aeshandle, FALSE);
	udef_vsm_color(aeshandle, G_BLACK);
	udef_vsm_type(aeshandle, 1);
	udef_vsm_height(aeshandle, 1);
	basec = (pb->pb_obj - PANEL_FIRST) * 16;

	src.fd_addr = dat_table;
	src.fd_w = fonthdr.form_width << 3;
	src.fd_h = fonthdr.form_height;
	src.fd_wdwidth = (src.fd_w + 15) >> 4;
	src.fd_nplanes = 1;
	src.fd_stand = FALSE;
	src.fd_r1 = 0;
	src.fd_r2 = 0;
	src.fd_r3 = 0;
	colors[0] = G_BLACK;
	colors[1] = G_WHITE;
	/* we can only use vro_cpyfm if form_width is even number of bytes */
	can_use_vrocpy = (src.fd_wdwidth << 1) == fonthdr.form_width;
	
	if (!can_use_vrocpy)
	{
		pxy[0] = pb->pb_x;
		pxy[1] = pb->pb_y;
		pxy[2] = pb->pb_x + pb->pb_w - 1;
		pxy[3] = pb->pb_y + pb->pb_h - 1;
		udef_vr_recfl(aeshandle, pxy);
	}
		
	for (c = 0; c < 16; c++)
	{
		ch = basec + c;
		if (ch < fonthdr.first_ade || ch > fonthdr.last_ade)
			continue;
		ch -= fonthdr.first_ade;
		o = off_table[ch];
		width = off_table[ch + 1] - o;
		if (can_use_vrocpy)
		{
			pxy[0] = o;
			pxy[1] = 0;
			pxy[2] = o + width - 1;
			pxy[3] = fonthdr.form_height - 1;
			pxy[4] = pb->pb_x + c * font_cw;
			pxy[5] = pb->pb_y;
			pxy[6] = pxy[4] + width - 1;
			pxy[7] = pxy[5] + fonthdr.form_height - 1;
			
			if (screen_fdb.fd_nplanes == 1)
				udef_vro_cpyfm(aeshandle, S_ONLY, pxy, &src, &screen_fdb);
			else
				udef_vrt_cpyfm(aeshandle, MD_REPLACE, pxy, &src, &screen_fdb, colors);
		} else
		{
			_WORD x, y;
			
			for (y = 0; y < font_ch; y++)
			{
				for (x = 0; x < width; x++)
				{
					if (char_testbit(ch, x, y))
					{
						pxy[0] = pb->pb_x + c * font_cw + x;
						pxy[1] = pb->pb_y + y;
						udef_v_pmarker(aeshandle, 1, pxy);
					}
				}
			}
		}
	}		

	udef_vst_color(aeshandle, tattrib[1]);
	udef_vst_rotation(aeshandle, tattrib[2]);
	udef_vst_alignment(aeshandle, tattrib[3], tattrib[4], &dummy, &dummy);
	udef_vst_height(aeshandle, tattrib[7], &dummy, &dummy, &dummy, &dummy);

	udef_vsf_interior(aeshandle, fattrib[0]);
	udef_vsf_color(aeshandle, fattrib[1]);
	udef_vsf_style(aeshandle, fattrib[2]);
	udef_vswr_mode(aeshandle, fattrib[3]);
	udef_vsf_perimeter(aeshandle, fattrib[4]);

	udef_vsm_type(aeshandle, mattrib[0]);
	udef_vsm_color(aeshandle, mattrib[1]);
	udef_vsm_height(aeshandle, mattrib[3]);
	
	return 0;
}

/* -------------------------------------------------------------------------- */

static USERBLK draw_font_userblk = { draw_font, 0 };

/*
 * resize the panel window, and install the
 * user-defined objects that do the actual drawing
 */
static void resize_panel(void)
{
	GRECT gr, desk;
	_WORD wkind;
	OBJECT *panel = rs_tree(PANEL);
	_WORD y;
	_WORD dummy;
	
	for (y = 0; y < 16; y++)
	{
		panel[PANEL_FIRST + y].ob_y = panel[PANEL_FIRST].ob_y + y * font_ch;
		panel[PANEL_FIRST + y].ob_height = font_ch;
		panel[PANEL_FIRST + y].ob_width = 16 * font_cw;
		panel[PANEL_FIRST + y].ob_type = G_USERDEF;
		panel[PANEL_FIRST + y].ob_spec.userblk = &draw_font_userblk;
	}
	panel[PANEL_BOX].ob_width = panel[PANEL_FIRST].ob_x * 2 + 16 * font_cw;
	panel[PANEL_BOX].ob_height = panel[PANEL_FIRST].ob_y * 2 + 16 * font_ch;
	panel[ROOT].ob_width = panel[PANEL_BOX].ob_width + 2 * panel[PANEL_BOX].ob_x;
	panel[ROOT].ob_height = panel[PANEL_BOX].ob_height + 2 * panel[PANEL_BOX].ob_y;
	form_center_grect(panel, &gr);
	set_panel_pos();
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = PANEL_WKIND;
	wind_get(panelwin, WF_WORKXYWH, &gr.g_x, &gr.g_y, &dummy, &dummy);
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);

	wind_set_grect(panelwin, WF_CURRXYWH, &gr);
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static void font_gethdr(FONT_HDR *hdr, const unsigned char *h)
{
	hdr->font_id = LOAD_W(h + 0);
	hdr->point = LOAD_W(h + 2);
	chomp(fontname, (const char *)h + 4, VDI_FONTNAMESIZE + 1);
	memcpy(hdr->name, fontname, VDI_FONTNAMESIZE);
	hdr->first_ade = LOAD_UW(h + 36);
	hdr->last_ade = LOAD_UW(h + 38);
	hdr->top = LOAD_UW(h + 40);
	hdr->ascent = LOAD_UW(h + 42);
	hdr->half = LOAD_UW(h + 44);
	hdr->descent = LOAD_UW(h + 46);
	hdr->bottom = LOAD_UW(h + 48);
	hdr->max_char_width = LOAD_UW(h + 50);
	hdr->max_cell_width = LOAD_UW(h + 52);
	hdr->left_offset = LOAD_UW(h + 54);
	hdr->right_offset = LOAD_UW(h + 56);
	hdr->thicken = LOAD_UW(h + 58);
	hdr->ul_size = LOAD_UW(h + 60);
	hdr->lighten = LOAD_UW(h + 62);
	hdr->skew = LOAD_UW(h + 64);
	hdr->flags = LOAD_UW(h + 66); 
	hdr->hor_table = LOAD_UL(h + 68);
	hdr->off_table = LOAD_UL(h + 72);
	hdr->dat_table = LOAD_UL(h + 76);
	hdr->form_width = LOAD_UW(h + 80);
	hdr->form_height = LOAD_UW(h + 82);
	hdr->next_font = 0;
}

/* -------------------------------------------------------------------------- */

static void font_puthdr(const FONT_HDR *hdr, unsigned char *h)
{
	STORE_W(h + 0, hdr->font_id);
	STORE_W(h + 2, hdr->point);
	memcpy(h + 4, hdr->name, VDI_FONTNAMESIZE);
	STORE_UW(h + 36, hdr->first_ade);
	STORE_UW(h + 38, hdr->last_ade);
	STORE_UW(h + 40, hdr->top);
	STORE_UW(h + 42, hdr->ascent);
	STORE_UW(h + 44, hdr->half);
	STORE_UW(h + 46, hdr->descent);
	STORE_UW(h + 48, hdr->bottom);
	STORE_UW(h + 50, hdr->max_char_width);
	STORE_UW(h + 52, hdr->max_cell_width);
	STORE_UW(h + 54, hdr->left_offset);
	STORE_UW(h + 56, hdr->right_offset);
	STORE_UW(h + 58, hdr->thicken);
	STORE_UW(h + 60, hdr->ul_size);
	STORE_UW(h + 62, hdr->lighten);
	STORE_UW(h + 64, hdr->skew);
	STORE_UW(h + 66, hdr->flags & ~FONTF_COMPRESSED);
	STORE_UL(h + 68, hdr->hor_table);
	STORE_UL(h + 72, hdr->off_table);
	STORE_UL(h + 76, hdr->dat_table);
	STORE_UW(h + 80, hdr->form_width);
	STORE_UW(h + 82, hdr->form_height);
	STORE_UL(h + 84, hdr->next_font);
}

/* -------------------------------------------------------------------------- */

static void swap_gemfnt_header(FONT_HDR *hdr, unsigned long l)
{
	if (l < SIZEOF_FONT_HDR)
		return;
	SWAP_W(hdr->font_id);
	SWAP_W(hdr->point);
	/* skip name */
	SWAP_W(hdr->first_ade);
	SWAP_W(hdr->last_ade);
	SWAP_W(hdr->top);
	SWAP_W(hdr->ascent);
	SWAP_W(hdr->half);
	SWAP_W(hdr->descent);
	SWAP_W(hdr->bottom);
	SWAP_W(hdr->max_char_width);
	SWAP_W(hdr->max_cell_width);
	SWAP_W(hdr->left_offset);
	SWAP_W(hdr->right_offset);
	SWAP_W(hdr->thicken);
	SWAP_W(hdr->ul_size);
	SWAP_W(hdr->lighten);
	SWAP_W(hdr->skew);
	SWAP_W(hdr->flags);
	SWAP_L(hdr->hor_table);
	SWAP_L(hdr->off_table);
	SWAP_L(hdr->dat_table);
	SWAP_W(hdr->form_width);
	SWAP_W(hdr->form_height);
}

/* -------------------------------------------------------------------------- */

/*
 * There are apparantly several fonts that have the Motorola flag set
 * but are stored in little-endian format.
 */
static _BOOL check_gemfnt_header(FONT_HDR *h, unsigned long l)
{
	UW firstc, lastc, points;
	UW cellwidth;
	UL dat_offset;
	UL off_table;
	
	if (l < SIZEOF_FONT_HDR)
		return FALSE;
	firstc = h->first_ade;
	lastc = h->last_ade;
	points = h->point;
	if (lastc == 256)
	{
		lastc = 255;
	}
	if (firstc >= 0x2000 || lastc >= 0xff00 || firstc > lastc)
		return FALSE;
	if (points >= 0x300)
		return FALSE;
	if (h->hor_table >= l)
		return FALSE;
	off_table = h->off_table;
	if (off_table < SIZEOF_FONT_HDR || (off_table + (lastc - firstc + 1) * 2) > l)
		return FALSE;
	dat_offset = h->dat_table;
	if (dat_offset < SIZEOF_FONT_HDR || dat_offset >= l)
		return FALSE;
	cellwidth = h->max_cell_width;
	if (cellwidth == 0)
		return FALSE;
	if (!(h->flags & FONTF_COMPRESSED))
	{
	UW form_width, form_height;
	form_width = h->form_width;
	form_height = h->form_height;
	if ((dat_offset + (size_t)form_width * form_height) > l)
		return FALSE;
	}

	h->last_ade = lastc;
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL font_get_tables(unsigned char **m, const char *filename, unsigned long l)
{
	FONT_HDR *hdr = &fonthdr;
	uint16_t *u;
	unsigned char *h = *m;
	uint32_t dat_offset, off_offset, hor_offset;
	int decode_ok = TRUE;
	uint16_t last_offset;
	char buf[256];
	int hortable_bytes;
	
	numoffs = hdr->last_ade - hdr->first_ade + 1;

	hor_offset = hdr->hor_table;
	off_offset = hdr->off_table;
	dat_offset = hdr->dat_table;

	if (!(hdr->flags & FONTF_COMPRESSED) && l > 0)
	{
		if ((dat_offset + (size_t)hdr->form_width * hdr->form_height) > l)
			form_alert(1, rs_str(AL_TRUNCATED));
	}
		
	if (!(hdr->flags & FONTF_COMPRESSED))
	{
		if (dat_offset > off_offset && (off_offset + (numoffs + 1) * 2) < dat_offset)
			nf_debugprintf("%s: warning: %s: gap of %lu bytes before data\n", program_name, filename, (unsigned long)dat_offset - (off_offset + (numoffs + 1) * 2));
	}

	if (hdr->flags & FONTF_COMPRESSED)
	{
		size_t offset;
		size_t font_file_data_size;
		size_t compressed_size;
		size_t form_size;
		unsigned char *compressed;
		
		offset = hor_offset;
		if (offset == 0)
			offset = off_offset;
		if (l > 152 && offset >= 152)
		{
			compressed_size = LOAD_UW(h + 150);
			if (HOST_BIG != FONT_BIG)
				compressed_size = cpu_swab16(compressed_size);
			compressed_size -= dat_offset - offset;
			offset = dat_offset;
		} else
		{
			offset = dat_offset;
			compressed_size = l - offset;
		}
		form_size = (size_t)hdr->form_width * hdr->form_height;
		font_file_data_size = form_size;
		if (font_file_data_size < compressed_size)
		{
			sprintf(buf, rs_str(AL_COMPRESSED_SIZE), (unsigned long)compressed_size, (unsigned long)font_file_data_size);
			form_alert(1, buf);
			decode_ok = FALSE;
		} else
		{
			*m = h = realloc(h, l - compressed_size + font_file_data_size);
			compressed = malloc(compressed_size);
			memcpy(compressed, h + offset, compressed_size);
			decode_gemfnt(h + offset, compressed, hdr->form_width, hdr->form_height);
			free(compressed);
		}
	}
	
	hor_table = (int8_t *)h + hor_offset;
	off_table = (uint16_t *)(h + off_offset);
	dat_table = h + dat_offset;

	hortable_bytes = 0;
	if ((hdr->flags & FONTF_HORTABLE) && hor_offset != 0 && hor_offset < off_offset)
	{
		if ((off_offset - hor_offset) >= (numoffs * 2))
			hortable_bytes = 2;
		else
			hortable_bytes = 1;
	}
		
	if (FONT_BIG != HOST_BIG)
	{
		for (u = off_table; u <= off_table + numoffs; u++)
		{
			SWAP_W(*u);
		}
		if (hortable_bytes == 2)
		{
			for (u = (uint16_t *)hor_table; u < (uint16_t *)hor_table + numoffs; u++)
			{
				SWAP_W(*u);
			}
		}
	}
	
	last_offset = off_table[numoffs];
	if ((((last_offset + 15) >> 4) << 1) != hdr->form_width)
		nf_debugprintf("%s: warning: %s: offset of last character %u does not match form_width %u\n", program_name, filename, last_offset, hdr->form_width);

	if ((hdr->flags & FONTF_HORTABLE) && hortable_bytes != 0)
	{
		hor_table = (int8_t *)h + hdr->hor_table;
	} else
	{
		if (hdr->flags & FONTF_HORTABLE)
		{
			form_alert(1, rs_str(AL_MISSING_HOR_TABLE));
			hdr->flags &= ~FONTF_HORTABLE;
		} else if (hortable_bytes != 0)
		{
			form_alert(1, rs_str(AL_MISSING_HOR_TABLE_FLAG));
		}
		hor_table = NULL;
		hdr->hor_table = 0;
	}

	font_cw = hdr->max_cell_width;
	font_ch = hdr->form_height;
	cur_char = 'A';
	
	if (hor_table)
	{
		form_alert(1, rs_str(AL_NO_OFFTABLE));
	}
	
	return decode_ok;
}

/* -------------------------------------------------------------------------- */

static _BOOL font_gen_gemfont(unsigned char **m, const char *filename, unsigned long l)
{
	FONT_HDR *hdr = &fonthdr;
	unsigned char *h = *m;
	
	font_gethdr(hdr, h);
	
	if (!check_gemfnt_header(hdr, l))
	{
		swap_gemfnt_header(hdr, l);
		if (!check_gemfnt_header(hdr, l))
		{
			swap_gemfnt_header(hdr, l);
			form_alert(1, rs_str(AL_NOGEMFONT));
			return FALSE;
		} else
		{
			if (FONT_BIG)
			{
				form_alert(1, rs_str(AL_ENDIAN_FLAG));
				if (HOST_BIG)
				{
					/*
					 * host big-endian, font claims to be big-endian,
					 * but check succeded only after swapping:
					 * font apparently is little-endian, clear flag
					 */
					hdr->flags &= ~FONTF_BIGENDIAN;
				} else
				{
					/*
					 * host little-endian, font claims to be big-endian,
					 * but check succeded only after swapping:
					 * font apparently is little-endian, clear flag
					 */
					SM_UW(h + 66, LM_UW(h + 66) & ~FONTF_BIGENDIAN);
				}
			}
		}
	} else
	{
		if (!FONT_BIG)
		{
			form_alert(1, rs_str(AL_ENDIAN_FLAG));
			if (HOST_BIG)
			{
				/*
				 * host big-endian, font claims to be little-endian,
				 * but check succeded without swapping:
				 * font apparently is big-endian, set flag
				 */
				hdr->flags |= FONTF_BIGENDIAN;
			} else
			{
				/*
				 * host little-endian, font claims to be big-endian,
				 * but check succeded without swapping:
				 * font apparently is little-endian, clear flag
				 */
				hdr->flags &= ~FONTF_BIGENDIAN;
			}
		}
	}
	
	return font_get_tables(m, filename, l);
}

/* -------------------------------------------------------------------------- */

/*
 * update some global vars after a font has been loaded
 */
static void font_loaded(unsigned char *h, const char *filename)
{
#if 0
	nf_debugprintf("Filename: %s\n", filename);
	nf_debugprintf("Name: %s\n", fontname);
	nf_debugprintf("Id: %d\n", fonthdr.font_id);
	nf_debugprintf("Size: %dpt\n", fonthdr.point);
	nf_debugprintf("First ade: %d\n", fonthdr.first_ade);
	nf_debugprintf("Last ade: %d\n", fonthdr.last_ade);
	nf_debugprintf("Top: %d\n", fonthdr.top);
	nf_debugprintf("Ascent: %d\n", fonthdr.ascent);
	nf_debugprintf("Half: %d\n", fonthdr.half);
	nf_debugprintf("Descent: %d\n", fonthdr.descent);
	nf_debugprintf("Bottom: %d\n", fonthdr.bottom);
	nf_debugprintf("Max charwidth: %d\n", fonthdr.max_char_width);
	nf_debugprintf("Max cellwidth: %d\n", fonthdr.max_cell_width);
	nf_debugprintf("Left offset: %d\n", fonthdr.left_offset);
	nf_debugprintf("Right offset: %d\n", fonthdr.right_offset);
	nf_debugprintf("Thicken: %d\n", fonthdr.thicken);
	nf_debugprintf("Underline size: %d\n", fonthdr.ul_size);
	nf_debugprintf("Lighten: $%x\n", fonthdr.lighten);
	nf_debugprintf("Skew: $%x\n", fonthdr.skew);
	nf_debugprintf("Flags: $%x (%s%s%s-endian %s%s)\n", fonthdr.flags,
		fonthdr.flags & FONTF_SYSTEM ? "system " : "",
		fonthdr.flags & FONTF_HORTABLE ? "offsets " : "",
		fonthdr.flags & FONTF_BIGENDIAN ? "big" : "little",
		fonthdr.flags & FONTF_MONOSPACED ? "monospaced" : "proportional",
		fonthdr.flags & FONTF_EXTENDED ? " extended" : "");
	nf_debugprintf("Horizontal table: %lu\n", (unsigned long)fonthdr.hor_table);
	nf_debugprintf("Offset table: %lu\n", (unsigned long)fonthdr.off_table);
	nf_debugprintf("Data: %lu\n", (unsigned long)fonthdr.dat_table);
	nf_debugprintf("Form width: %d\n", fonthdr.form_width);
	nf_debugprintf("Form height: %d\n", fonthdr.form_height);
	nf_debugprintf("\n");
#endif

	free(fontmem);
	fontmem = h;
	last_display_cw = 0;
	maybe_resize_window();
	resize_panel();
	if (h)
	{
		wind_set_str(mainwin, WF_NAME, fontname);
		fontfilename = filename;
		fontbasename = xbasename(filename);
		wind_set_str(panelwin, WF_NAME, fontbasename);
	} else
	{
		wind_set_str(mainwin, WF_NAME, "");
		wind_set_str(panelwin, WF_NAME, "");
		fontfilename = NULL;
		fontbasename = NULL;
		dat_table = NULL;
		last_display_cw = 0;
	}
	redraw_win(mainwin);
	redraw_win(panelwin);
	font_changed = FALSE;
}

/* -------------------------------------------------------------------------- */

static _BOOL font_load_gemfont(const char *filename)
{
	FILE *in;
	unsigned long l;
	unsigned char *h;
	_BOOL ret;
	
	in = fopen(filename, "rb");
	if (in == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FOPEN), filename);
		form_alert(1, buf);
		return FALSE;
	}
	fseek(in, 0, SEEK_END);
	l = ftell(in);
	fseek(in, 0, SEEK_SET);
	h = malloc(l);
	if (h == NULL)
	{
		fclose(in);
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	l = fread(h, 1, l, in);

	ret = font_gen_gemfont(&h, filename, l);
	fclose(in);

	if (ret == FALSE)
	{
		free(h);
		h = NULL;
		filename = NULL;
	}
	font_loaded(h, filename);

	return ret;
}

/* -------------------------------------------------------------------------- */

#ifdef __PUREC__
static void push_a2(void) 0x2F0A;
static long pop_a2(void) 0x245F;
static void *get_a1(void) 0x2049;
static void *get_a2(void) 0x204A;

static void *linea0(void) 0xa000;

static void init_linea(void)
{
	push_a2();
#if 0
	Linea = linea0();
	Vdiesc = (VDIESC *)((char *)Linea - sizeof(VDIESC));
	Fonts = get_a1();
	Linea_funp = get_a2();
#else
	linea0();
	Fonts = get_a1();
#endif
	pop_a2();
}
#endif


#ifdef __GNUC__
static void init_linea(void)
{
	__asm__ __volatile__(
#ifdef __mcoldfire__
		"\tdc.w 0xa920\n"
#else
		"\tdc.w 0xa000\n"
#endif
		"\tmove.l %%a1,%0\n"
	: "=m"(Fonts)
	:
	: "d0", "d1", "d2", "a0", "a1", "a2", "cc", "memory"
	);
}
#endif

/* -------------------------------------------------------------------------- */

static _BOOL font_load_sysfont(int fontnum)
{
	FONT_HDR *hdr = &fonthdr;
	const unsigned char *h;
	unsigned char *m;
	const char *filename =
		fontnum == 0 ? "system0.fnt" :
		fontnum == 1 ? "system1.fnt" :
		fontnum == 3 ? "system3.fnt" :
		"system2.fnt";
	unsigned long l;
	size_t offtable_size;
	size_t form_size;
	
	if (font_changed)
	{
		if (form_alert(1, rs_str(AL_CHANGED)) != 2)
			return FALSE;
	}
	
	init_linea();
	
	h = (const unsigned char *)(Fonts->font[fontnum]);
	
	font_gethdr(hdr, h);
	m = NULL;
	font_get_tables(&m, filename, 0);
	offtable_size = (size_t)(numoffs + 1) * 2;
	form_size = (size_t)hdr->form_width * (size_t)hdr->form_height;
	l = SIZEOF_FONT_HDR + offtable_size + form_size;

	m = malloc(l);
	if (m == NULL)
	{
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	memcpy(m, h, SIZEOF_FONT_HDR);
	memcpy(m + SIZEOF_FONT_HDR, off_table, offtable_size);
	memcpy(m + SIZEOF_FONT_HDR + offtable_size, dat_table, form_size);
	
	off_table = (uint16_t *)(m + SIZEOF_FONT_HDR);
	dat_table = m + SIZEOF_FONT_HDR + offtable_size;
	
	font_loaded(m, filename);

	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL do_fsel_input(char *path, char *filename, char *mask, const char *title)
{
	_WORD button = 0;
	_WORD ret;
	char *p;
	
	wind_update(BEG_UPDATE);
	p = xbasename(path);
	strcpy(p, mask);
	if (gl_ap_version >= 0x0140)
		ret = fsel_exinput(path, filename, &button, title);
	else
		ret = fsel_input(path, filename, &button);
	wind_update(END_UPDATE);
	if (ret == 0 || button == 0)
		return FALSE;
	p = xbasename(path);
	strcpy(mask, p);
	strcpy(p, filename);
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static void select_font(void)
{
	char filename[128];
	static char path[128];
	static char mask[128] = "*.FNT";
	
	if (font_changed)
	{
		if (form_alert(1, rs_str(AL_CHANGED)) != 2)
			return;
	}
	
	if (path[0] == '\0')
	{
		path[0] = Dgetdrv() + 'A';
		path[1] = ':';
		Dgetpath(path + 2, 0);
		strcat(path, "\\");
	}
	strcpy(filename, "");
	
	if (!do_fsel_input(path, filename, mask, rs_str(SEL_FONT)))
		return;
	font_load_gemfont(path);
}

/* -------------------------------------------------------------------------- */

static _BOOL font_save_gemfont(const char *filename)
{
	FONT_HDR *hdr = &fonthdr;
	FILE *fp;
	size_t offtable_size;
	size_t form_size;
	unsigned char h[SIZEOF_FONT_HDR];
	uint16_t *u;
	_BOOL swapped;
	
	fp = fopen(filename, "rb");
	if (fp != NULL)
	{
		fclose(fp);
		if (form_alert(1, rs_str(AL_EXISTS)) != 2)
			return FALSE;
	}
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FCREATE), filename);
		form_alert(1, buf);
		return FALSE;
	}
	
	offtable_size = (size_t)(numoffs + 1) * 2;
	form_size = (size_t)hdr->form_width * (size_t)hdr->form_height;

	hdr->next_font = 0;
	hdr->hor_table = 0;
	hdr->off_table = sizeof(h);
	hdr->dat_table = hdr->off_table + offtable_size;
	
	swapped = FONT_BIG != HOST_BIG;
	if (swapped)
	{
		swap_gemfnt_header(hdr, sizeof(*hdr));
		for (u = off_table; u <= off_table + numoffs; u++)
		{
			SWAP_W(*u);
		}
	}

	font_puthdr(hdr, h);
	
	fwrite(h, 1, sizeof(h), fp);
	fwrite(off_table, 1, offtable_size, fp);
	fwrite(dat_table, 1, form_size, fp);
	
	fclose(fp);
	
	if (swapped)
	{
		swap_gemfnt_header(hdr, sizeof(*hdr));
		for (u = off_table; u <= off_table + numoffs; u++)
		{
			SWAP_W(*u);
		}
	}

	fontfilename = filename;
	fontbasename = xbasename(filename);
	wind_set_str(panelwin, WF_NAME, fontbasename);

	font_changed = FALSE;
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static void save_font(const char *filename)
{
	static char path[128];
	static char mask[128] = "*.FNT";
	char filename_buf[128];
	
	if (!is_font_loaded())
		return;
	if (path[0] == '\0')
	{
		path[0] = Dgetdrv() + 'A';
		path[1] = ':';
		Dgetpath(path + 2, 0);
		strcat(path, "\\");
	}
	if (filename)
	{
		if (isalpha(filename[0]) && filename[1] == ':')
			strcpy(path, filename);
		else
			strcpy(xbasename(path), filename);
	}
	strcpy(filename_buf, "");
	
	if (!do_fsel_input(path, filename_buf, mask, rs_str(SEL_OUTPUT)))
		return;
	font_save_gemfont(path);
}

/* -------------------------------------------------------------------------- */

static _BOOL font_export_as_c(const char *filename)
{
	FONT_HDR *hdr = &fonthdr;
	FILE *fp;
	fchar_t i, end;
	
	fp = fopen(filename, "rb");
	if (fp != NULL)
	{
		fclose(fp);
		if (form_alert(1, rs_str(AL_EXISTS)) != 2)
			return FALSE;
	}
	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FCREATE), filename);
		form_alert(1, buf);
		return FALSE;
	}
	
	fprintf(fp, "\
/*\n\
 * %s - a font in standard format\n\
 *\n\
 * Automatically generated by %s\n\
 */\n", xbasename(filename), program_name);
 
	fprintf(fp, "\
\n\
#include \"portab.h\"\n\
#include \"fonthdr.h\"\n\
\n");

	fprintf(fp, "static UWORD const off_table[] =\n{\n");
	end = hdr->last_ade + 2;
	for (i = hdr->first_ade; i < end; i++)
	{
		if ((i & 7) == 0)
			fprintf(fp, "    ");
		else
			fprintf(fp, " ");
		fprintf(fp, "0x%04x", off_table[i]);
		if (i != (end - 1))
			fprintf(fp, ",");
		if ((i & 7) == 7)
			fprintf(fp, "\n");
	}
	if ((i & 7) != 0)
		fprintf(fp, "\n");
	fprintf(fp, "};\n\n");

	fprintf(fp, "static UWORD const dat_table[] =\n{\n");
	{
		size_t j, h;
		uint16_t a;
		
		h = ((size_t)hdr->form_height * hdr->form_width) / 2;
		for (j = 0; j < h; j++)
		{
			if ((j & 7) == 0)
				fprintf(fp, "    ");
			else
				fprintf(fp, " ");
			a = (dat_table[2 * j] << 8) | (dat_table[2 * j + 1] & 0xFF);
			fprintf(fp, "0x%04x", a);
			if (j != (h - 1))
				fprintf(fp, ",");
			if ((j & 7) == 7)
				fprintf(fp, "\n");
		}
		if ((j & 7) != 0)
			fprintf(fp, "\n");
	}
	fprintf(fp, "};\n\n");

	fprintf(fp, "struct FONT_HDR const THISFONT = {\n");

#define SET_WORD(a)  fprintf(fp, "    %u,  /* " #a " */\n", hdr->a)
#define SET_UWORD(a) fprintf(fp, "    0x%04x,  /* " #a " */\n", hdr->a)

	SET_WORD(font_id);
	SET_WORD(point);
	fprintf(fp, "    \"");
	for (i = 0; i < VDI_FONTNAMESIZE; i++)
	{
		char c = fontname[i];

		if (c == 0)
			break;
		if (c < 32 || c > 126 || c == '\\' || c == '"')
		{
			fprintf(fp, "\\%03o", c);
		} else
		{
			fprintf(fp, "%c", c);
		}
	}

	fprintf(fp, "\",  /*   BYTE name[32]	*/\n");

	SET_WORD(first_ade);
	SET_WORD(last_ade);
	SET_WORD(top);
	SET_WORD(ascent);
	SET_WORD(half);
	SET_WORD(descent);
	SET_WORD(bottom);
	SET_WORD(max_char_width);
	SET_WORD(max_cell_width);
	SET_WORD(left_offset);
	SET_WORD(right_offset);
	SET_WORD(thicken);
	SET_WORD(ul_size);

	SET_UWORD(lighten);
	SET_UWORD(skew);
	hdr->flags &= ~FONTF_COMPRESSED;
	SET_UWORD(flags);
	fprintf(fp, "    0,			/*   UBYTE *hor_table	*/\n");
	fprintf(fp, "    off_table,		/*   UWORD *off_table	*/\n");
	fprintf(fp, "    dat_table,		/*   UWORD *dat_table	*/\n");

	SET_WORD(form_width);
	SET_WORD(form_height);
	fprintf(fp, "    0,  /* struct font * next_font */\n");
	fprintf(fp, "    0   /* UWORD next_seg */\n");
	fprintf(fp, "};\n\n");

#undef SET_WORD
#undef SET_UWORD

	fclose(fp);
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL font_export_as_txt(const char *filename)
{
	FONT_HDR *hdr = &fonthdr;
	FILE *fp;
	fchar_t i;
	
	fp = fopen(filename, "rb");
	if (fp != NULL)
	{
		fclose(fp);
		if (form_alert(1, rs_str(AL_EXISTS)) != 2)
			return FALSE;
	}
	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FCREATE), filename);
		form_alert(1, buf);
		return FALSE;
	}
	
	fprintf(fp, "GDOSFONT\n");
	fprintf(fp, "version 1.0\n");

#define SET_WORD(a)  fprintf(fp, #a " %u\n", hdr->a)
#define SET_UWORD(a) fprintf(fp, #a " 0x%04x\n", hdr->a)

	SET_WORD(font_id);
	SET_WORD(point);

	fprintf(fp, "name \"");
	for (i = 0; i < VDI_FONTNAMESIZE; i++)
	{
		char c = hdr->name[i];

		if (c == 0)
			break;
		if (c < 32 || c > 126 || c == '\\' || c == '"')
		{
			fprintf(fp, "\\%03o", c);
		} else
		{
			fprintf(fp, "%c", c);
		}
	}
	fprintf(fp, "\"\n");

	SET_WORD(first_ade);
	SET_WORD(last_ade);
	SET_WORD(top);
	SET_WORD(ascent);
	SET_WORD(half);
	SET_WORD(descent);
	SET_WORD(bottom);
	SET_WORD(max_char_width);
	SET_WORD(max_cell_width);
	SET_WORD(left_offset);
	SET_WORD(right_offset);
	SET_WORD(thicken);
	SET_WORD(ul_size);

	SET_UWORD(lighten);
	SET_UWORD(skew);
	hdr->flags &= ~FONTF_COMPRESSED;
	SET_UWORD(flags);

	SET_WORD(form_height);

#undef SET_WORD
#undef SET_UWORD

	/* then, output char bitmaps */
	for (i = 0; i < numoffs; i++)
	{
		uint16_t y, x, w, off;
		fchar_t c;
		
		c = i + hdr->first_ade;
		off = off_table[i];
		w = off_table[i + 1] - off_table[i];
		if (off + w > 8 * hdr->form_width)
		{
			nf_debugprintf("char %d: offset %d + width %d out of range (%d)\n", c, off, w, 8 * hdr->form_width);
			continue;
		}
		if (c < 32 || c > 126)
		{
			fprintf(fp, "char 0x%02x\n", c);
		} else if (c == '\\' || c == '\'')
		{
			fprintf(fp, "char '\\%c'\n", c);
		} else
		{
			fprintf(fp, "char '%c'\n", c);
		}

		for (y = 0; y < hdr->form_height; y++)
		{
			for (x = 0; x < w; x++)
			{
				if (char_testbit(c, x, y))
				{
					fprintf(fp, "X");
				} else
				{
					fprintf(fp, ".");
				}
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "endchar\n");
	}
	fprintf(fp, "endfont\n");

	fclose(fp);
	
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static void export_font_c(void)
{
	static char path[128];
	static char mask[128] = "*.C";
	char filename_buf[128];
	
	if (!is_font_loaded())
		return;
	if (path[0] == '\0')
	{
		path[0] = Dgetdrv() + 'A';
		path[1] = ':';
		Dgetpath(path + 2, 0);
		strcat(path, "\\");
	}
	strcpy(filename_buf, "");
	
	if (!do_fsel_input(path, filename_buf, mask, rs_str(SEL_OUTPUT)))
		return;
	font_export_as_c(path);
}

/* -------------------------------------------------------------------------- */

static void export_font_txt(void)
{
	static char path[128];
	static char mask[128] = "*.TXT";
	char filename_buf[128];
	
	if (!is_font_loaded())
		return;
	if (path[0] == '\0')
	{
		path[0] = Dgetdrv() + 'A';
		path[1] = ':';
		Dgetpath(path + 2, 0);
		strcat(path, "\\");
	}
	strcpy(filename_buf, "");
	
	if (!do_fsel_input(path, filename_buf, mask, rs_str(SEL_OUTPUT)))
		return;
	font_export_as_txt(path);
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

/* returns the next logical char, in sh syntax */
static int inextsh(FILE *f, int *lineno)
{
	int ret;

	ret = getc(f);
	if (ret == 015)
	{
		ret = getc(f);
		if (ret == 012)
		{
			(*lineno)++;
			return '\n';
		} else
		{
			if (ret != EOF)
				ungetc(ret, f);
			return 015;
		}
	} else if (ret == 012)
	{
		(*lineno)++;
		return '\n';
	} else
	{
		return ret;
	}
}

/* -------------------------------------------------------------------------- */

/* read a line, ignoring comments, initial white, trailing white,
 * and long lines.
 * Note that empty lines are not ignored,
 * because there might be characters without a bitmap.
 */
static _BOOL igetline(FILE *f, char *buf, int max, int *lineno)
{
	char *b;
	char *bmax = buf + max - 1;
	int c;

  again:
	c = inextsh(f, lineno);
	b = buf;
	if (c == '#')
	{
	  ignore:
		while (c != EOF && c != '\n')
		{
			c = inextsh(f, lineno);
		}
		goto again;
	}
	while (c == ' ' || c == '\t')
	{
		c = inextsh(f, lineno);
	}
	while (c != EOF && c != '\n')
	{
		if (b >= bmax)
		{
			nf_debugprintf("line %d too long\n", *lineno);
			goto ignore;
		}
		*b++ = c;
		c = inextsh(f, lineno);
	}
	/* remove trailing white */
	b--;
	while (b >= buf && (*b == ' ' || *b == '\t'))
	{
		b--;
	}
	b++;
	*b = 0;
	if (b == buf)
	{
		if (c == EOF)
			return FALSE;						/* EOF */
	}
	return TRUE;
}

/* -------------------------------------------------------------------------- */

/*
 * functions to try read some patterns.
 * they look in a string, return 1 if read, 0 if not read.
 * if the pattern was read, the string pointer is set to the
 * character immediately after the last character of the pattern.
 */

/* backslash sequences in C strings */
static _BOOL try_backslash(char **cc, long *val)
{
	long ret;
	char *c = *cc;

	if (*c++ != '\\')
		return FALSE;
	switch (*c)
	{
	case 0:
		return FALSE;
	case 'a':
		ret = '\a';
		c++;
		break;
	case 'b':
		ret = '\b';
		c++;
		break;
	case 'f':
		ret = '\f';
		c++;
		break;
	case 'n':
		ret = '\n';
		c++;
		break;
	case 'r':
		ret = '\r';
		c++;
		break;
	case 't':
		ret = '\t';
		c++;
		break;
	case 'v':
		ret = '\v';
		c++;
		break;
	case '\\':
	case '\'':
	case '\"':
		ret = *c++;
		break;
	default:
		if (*c >= '0' && *c <= '7')
		{
			ret = *c++ - '0';
			if (*c >= '0' && *c <= '7')
			{
				ret <<= 3;
				ret |= *c++ - '0';
				if (*c >= '0' && *c <= '7')
				{
					ret <<= 3;
					ret |= *c++ - '0';
				}
			}
		} else
		{
			ret = *c++;
		}
		break;
	}
	*cc = c;
	*val = ret;
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL try_unsigned(char **cc, long *val)
{
	long ret;
	char *c = *cc;

	if (*c == '0')
	{
		c++;
		if (*c == 'x')
		{
			c++;
			ret = 0;
			if (*c == 0)
				return FALSE;
			while (*c)
			{
				if (*c >= '0' && *c <= '9')
				{
					ret <<= 4;
					ret |= (*c - '0');
				} else if (*c >= 'a' && *c <= 'f')
				{
					ret <<= 4;
					ret |= (*c - 'a' + 10);
				} else if (*c >= 'A' && *c <= 'F')
				{
					ret <<= 4;
					ret |= (*c - 'A' + 10);
				} else
					break;
				c++;
			}
		} else
		{
			ret = 0;
			while (*c >= '0' && *c <= '7')
			{
				ret <<= 3;
				ret |= (*c++ - '0');
			}
		}
	} else if (*c >= '1' && *c <= '9')
	{

		ret = 0;
		while (*c >= '0' && *c <= '9')
		{
			ret *= 10;
			ret += (*c++ - '0');
		}
	} else if (*c == '\'')
	{
		c++;
		if (try_backslash(&c, &ret))
		{
			if (*c++ != '\'')
				return 0;
		} else if (*c == '\'' || *c < 32 || *c >= 127)
		{
			return 0;
		} else
		{
			ret = (*c++) & 0xFF;
			if (*c++ != '\'')
				return 0;
		}
	} else
	{
		return FALSE;
	}
	*cc = c;
	*val = ret;
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL try_given_string(char **cc, char *s)
{
	size_t n = strlen(s);

	if (strncmp(*cc, s, n) == 0)
	{
		*cc += n;
		return TRUE;
	}
	return FALSE;
}

/* -------------------------------------------------------------------------- */

static _BOOL try_c_string(char **cc, char *s, int max)
{
	char *c = *cc;
	char *smax = s + max - 1;
	long u;

	if (*c != '"')
	{
		/* fprintf(stderr, "c='%c'\n", *c); */
		return FALSE;
	}
	c++;
	while (*c != '"')
	{
		if (*c == 0)
		{
			/* fprintf(stderr, "c='%c'\n", *c); */
			return FALSE;
		}
		if (s >= smax)
		{
			/* fprintf(stderr, "c='%c'\n", *c); */
			return FALSE;
		}
		if (try_backslash(&c, &u))
		{
			*s++ = u;
		} else
		{
			*s++ = *c++;
		}
	}
	c++;
	*s++ = 0;
	*cc = c;
	return TRUE;
}

/* -------------------------------------------------------------------------- */

static _BOOL try_white(char **cc)
{
	char *c = *cc;

	if (*c == ' ' || *c == '\t')
	{
		c++;
		while (*c == ' ' || *c == '\t')
			c++;
		*cc = c;
		return TRUE;
	}
	return FALSE;
}

/* -------------------------------------------------------------------------- */

static _BOOL try_eol(char **cc)
{
	return (**cc == 0) ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------- */

/*
 * simple bitmap read/write
 */

static _BOOL get_bit(unsigned char *addr, size_t i)
{
	return (addr[i / 8] & (1 << (7 - (i & 7)))) ? 1 : 0;
}

static void set_bit(unsigned char *addr, size_t i)
{
	addr[i / 8] |= (1 << (7 - (i & 7)));
}

/* -------------------------------------------------------------------------- */

static _BOOL font_import_from_txt(const char *filename)
{
	int lineno = 0;
	FILE *f;
	char line[200];
	char *c;
	long u;
	const int max = (int)sizeof(line);
	FONT_HDR p;
	char buf[256];
	unsigned long l;
	size_t offtable_size;
	size_t bmsize;
	fchar_t bmnum;
	unsigned char *h = NULL;
	fchar_t i;
	fchar_t ch;
	uint16_t *off_tab = NULL;
	unsigned char *b;
	unsigned char *bms;
	unsigned short w, width;
	size_t k;
	unsigned short j;
	unsigned short o;
	
	f = fopen(filename, "rb");
	if (f == NULL)
	{
		sprintf(buf, rs_str(AL_FOPEN), filename);
		form_alert(1, buf);
		return FALSE;
	}

#define EXPECT(a) \
	if(!igetline(f, line, max, &lineno) || strcmp(line, a) != 0) goto fail;

	EXPECT("GDOSFONT");
	EXPECT("version 1.0");

#define EXPECTNUM(a) c=line; \
	if (!igetline(f, line, max, &lineno) || \
		!try_given_string(&c, #a) || \
		!try_white(&c) || \
		!try_unsigned(&c, &u) || \
		!try_eol(&c)) \
		goto fail; \
	p.a = u;

	EXPECTNUM(font_id);
	EXPECTNUM(point);

	c = line;
	if (!igetline(f, line, max, &lineno) ||
		!try_given_string(&c, "name") ||
		!try_white(&c) ||
		!try_c_string(&c, p.name, VDI_FONTNAMESIZE) ||
		!try_eol(&c))
		goto fail;

	EXPECTNUM(first_ade);
	EXPECTNUM(last_ade);
	EXPECTNUM(top);
	EXPECTNUM(ascent);
	EXPECTNUM(half);
	EXPECTNUM(descent);
	EXPECTNUM(bottom);
	EXPECTNUM(max_char_width);
	EXPECTNUM(max_cell_width);
	EXPECTNUM(left_offset);
	EXPECTNUM(right_offset);
	EXPECTNUM(thicken);
	EXPECTNUM(ul_size);
	EXPECTNUM(lighten);
	EXPECTNUM(skew);
	EXPECTNUM(flags);
	EXPECTNUM(form_height);

#undef EXPECT
#undef EXPECTNUM

	if (p.first_ade > 255 || p.last_ade > 255 || p.first_ade > p.last_ade)
	{
		sprintf(buf, rs_str(AL_CHAR_RANGE), p.first_ade, p.last_ade);
		goto error;
	}
	
	if (p.max_cell_width == 0 || p.max_cell_width > 1000 || p.form_height == 0 || p.form_height > 1000)
	{
		sprintf(buf, rs_str(AL_FONT_SIZE), p.max_cell_width, p.form_height);
		goto error;
	}
	
	/*
	 * allocate a temporary buffer big enough to hold the
	 * the offset table and all bitmaps.
	 * The bitmaps are organized per char here.
	 */
	bmnum = p.last_ade - p.first_ade + 1;
	offtable_size = (size_t)(bmnum + 1) * 2;
	bmsize = (((size_t)p.max_cell_width + 7) >> 3) * (size_t)p.form_height;
	l = offtable_size + bmsize * bmnum;
	off_tab = malloc(l);
	if (off_tab == NULL)
	{
		fclose(f);
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	memset(off_tab, 0, l);
	bms = (unsigned char *)off_tab + offtable_size;
	
	for (i = 0; i < bmnum; i++)
	{
		off_tab[i] = F_NO_CHAR;
	}

	for (;;)
	{									/* for each char */
		c = line;
		if (!igetline(f, line, max, &lineno))
			goto fail;
		if (*line == '\0')
			continue;
		if (strcmp(line, "endfont") == 0)
			break;
		if (!try_given_string(&c, "char") || !try_white(&c) || !try_unsigned(&c, &u) || !try_eol(&c))
			goto fail;
		ch = u;
		if (ch < p.first_ade || ch > p.last_ade)
		{
			sprintf(buf, rs_str(AL_WRONG_CHAR), ch, lineno);
			goto error;
		}

		ch -= p.first_ade;
		if (off_tab[ch] != F_NO_CHAR)
		{
			sprintf(buf, "[1][Character number %u|was already defined][Abort]", ch + p.first_ade);
			goto error;
		}
		b = bms + ch * bmsize;

		k = 0;
		width = 0;
		for (i = 0; i < p.form_height; i++)
		{
			if (!igetline(f, line, max, &lineno))
				goto fail;
			for (c = line, w = 0; *c; c++, w++)
			{
				if (w >= p.max_cell_width)
				{
					sprintf(buf, rs_str(AL_LINE_TOO_LONG), lineno);
					goto error;
				} else if (*c == 'X')
				{
					set_bit(b, k);
				} else if (*c == '.')
				{
				} else
				{
					goto fail;
				}
				k++;
			}
			if (i == 0)
			{
				width = w;
			} else if (w != width)
			{
				sprintf(buf, rs_str(AL_DIFFERENT_LENGTH), lineno);
				goto error;
			}
		}
		if (!igetline(f, line, max, &lineno) || strcmp(line, "endchar") != 0)
			goto fail;
		off_tab[ch] = width;
	}
	fclose(f);

	/* compute size of final form, and compute offs from widths */
	o = 0;
	for (i = 0; i < bmnum; i++)
	{
		w = off_tab[i];
		off_tab[i] = o;
		if (w != F_NO_CHAR)
			o += w;
	}
	off_tab[i] = o;

	/*
	 * allocate a buffer big enough to hold the
	 * font header, the offset table and all bitmaps
	 */
	/*
	 * form_width seems to be always rounded up to an even number of bytes
	 */
	p.form_width = ((o + 15) >> 4) << 1;
	l = SIZEOF_FONT_HDR + offtable_size + (size_t)p.form_width * p.form_height;
	h = malloc(l);
	if (h == NULL)
	{
		free(off_tab);
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	memset(h, 0, l);

	off_table = (uint16_t *)(h + SIZEOF_FONT_HDR);
	dat_table = h + SIZEOF_FONT_HDR + offtable_size;
	
	/* now, pack the bitmaps in the destination form */
	for (i = 0; i < bmnum; i++)
	{
		o = off_tab[i];
		off_table[i] = o;
		width = off_tab[i + 1] - o;
		b = bms + bmsize * i;
		k = 0;
		for (j = 0; j < p.form_height; j++)
		{
			for (w = 0; w < width; w++)
			{
				if (get_bit(b, k))
				{
					set_bit(dat_table + (size_t)j * p.form_width, o + w);
				}
				k++;
			}
		}
	}

	/* copy last offset */
	o = off_tab[i];
	off_table[i] = o;

	/* updated remaining vars */
	p.off_table = SIZEOF_FONT_HDR;
	p.dat_table = SIZEOF_FONT_HDR + offtable_size;
	p.hor_table = 0;
	if (HOST_BIG)
		p.flags |= FONTF_BIGENDIAN;
	else
		p.flags &= ~FONTF_BIGENDIAN;
	p.flags &= ~FONTF_COMPRESSED;
	fonthdr = p;
	font_puthdr(&p, h);
	chomp(fontname, p.name, VDI_FONTNAMESIZE + 1);
	font_get_tables(&h, filename, 0);
	
	/* free temporary form */
	free(off_tab);

	font_loaded(h, filename);
	
	return TRUE;

fail:
	sprintf(buf, rs_str(AL_FIMPORT), xbasename(filename), lineno);
error:
	fclose(f);
	free(off_tab);
	form_alert(1, buf);

	return FALSE;
}

/* -------------------------------------------------------------------------- */

static void import_font_txt(void)
{
	static char path[128];
	static char mask[128] = "*.TXT";
	char filename_buf[128];
	
	if (path[0] == '\0')
	{
		path[0] = Dgetdrv() + 'A';
		path[1] = ':';
		Dgetpath(path + 2, 0);
		strcat(path, "\\");
	}
	strcpy(filename_buf, "");
	
	if (!do_fsel_input(path, filename_buf, mask, rs_str(SEL_INPUT)))
		return;
	font_import_from_txt(path);
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static void EnableObjState(OBJECT *tree, _WORD idx, _WORD state, _BOOL enable)
{
	if (enable)
		tree[idx].ob_state |= state;
	else
		tree[idx].ob_state &= ~state;
}

/* -------------------------------------------------------------------------- */

static _WORD do_dialog(_WORD num)
{
	OBJECT *tree = rs_tree(num);
	GRECT gr;
	_WORD ret;

	wind_update(BEG_UPDATE);
	form_center_grect(tree, &gr);
	form_dial_grect(FMD_START, &gr, &gr);
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &gr);
	ret = form_do(tree, ROOT);
	ret &= 0x7fff;
	tree[ret].ob_state &= ~OS_SELECTED;
	form_dial_grect(FMD_FINISH, &gr, &gr);
	wind_update(END_UPDATE);
	return ret;
}

/* -------------------------------------------------------------------------- */

static void font_info(void)
{
	FONT_HDR *hdr = &fonthdr;
	OBJECT *tree = rs_tree(FONT_PARAMS);
	_WORD ret;
	
	if (!is_font_loaded())
		return;
	
	strcpy(tree[FONT_NAME].ob_spec.tedinfo->te_ptext, fontname);
	sprintf(tree[FONT_ID].ob_spec.tedinfo->te_ptext, "%5d", hdr->font_id);
	sprintf(tree[FONT_POINT].ob_spec.tedinfo->te_ptext, "%5d", hdr->point);
	sprintf(tree[FONT_TOP].ob_spec.tedinfo->te_ptext, "%3d", hdr->top);
	sprintf(tree[FONT_ASCENT].ob_spec.tedinfo->te_ptext, "%3d", hdr->ascent);
	sprintf(tree[FONT_HALF].ob_spec.tedinfo->te_ptext, "%3d", hdr->half);
	sprintf(tree[FONT_DESCENT].ob_spec.tedinfo->te_ptext, "%3d", hdr->descent);
	sprintf(tree[FONT_BOTTOM].ob_spec.tedinfo->te_ptext, "%3d", hdr->descent);
	sprintf(tree[FONT_HEIGHT].ob_spec.tedinfo->te_ptext, "%3d", hdr->form_height);
	sprintf(tree[FONT_WIDTH].ob_spec.tedinfo->te_ptext, "%3d", hdr->max_cell_width);
	sprintf(tree[FONT_FIRST_ADE].ob_spec.tedinfo->te_ptext, "%3d", hdr->first_ade);
	sprintf(tree[FONT_LAST_ADE].ob_spec.tedinfo->te_ptext, "%3d", hdr->last_ade);
	
	EnableObjState(tree, FONT_SYSTEM, OS_SELECTED, (hdr->flags & FONTF_SYSTEM) != 0);
	EnableObjState(tree, FONT_HORTABLE, OS_SELECTED, (hdr->flags & FONTF_HORTABLE) != 0);
	EnableObjState(tree, FONT_BIGENDIAN, OS_SELECTED, (hdr->flags & FONTF_BIGENDIAN) != 0);
	EnableObjState(tree, FONT_MONOSPACED, OS_SELECTED, (hdr->flags & FONTF_MONOSPACED) != 0);
	EnableObjState(tree, FONT_COMPRESSED, OS_SELECTED, (hdr->flags & FONTF_COMPRESSED) != 0);
	
	ret = do_dialog(FONT_PARAMS);
	
	if (ret == FONT_OK)
	{
		chomp(fontname, tree[FONT_NAME].ob_spec.tedinfo->te_ptext, VDI_FONTNAMESIZE + 1);
		memcpy(hdr->name, fontname, VDI_FONTNAMESIZE);
		hdr->font_id = atoi(tree[FONT_ID].ob_spec.tedinfo->te_ptext);
		hdr->point = atoi(tree[FONT_POINT].ob_spec.tedinfo->te_ptext);
		hdr->top = atoi(tree[FONT_TOP].ob_spec.tedinfo->te_ptext);
		hdr->ascent = atoi(tree[FONT_ASCENT].ob_spec.tedinfo->te_ptext);
		hdr->half = atoi(tree[FONT_HALF].ob_spec.tedinfo->te_ptext);
		hdr->descent = atoi(tree[FONT_DESCENT].ob_spec.tedinfo->te_ptext);
		hdr->bottom = atoi(tree[FONT_BOTTOM].ob_spec.tedinfo->te_ptext);
		hdr->form_height = atoi(tree[FONT_HEIGHT].ob_spec.tedinfo->te_ptext);
		hdr->max_cell_width = atoi(tree[FONT_WIDTH].ob_spec.tedinfo->te_ptext);
		hdr->first_ade = atoi(tree[FONT_FIRST_ADE].ob_spec.tedinfo->te_ptext);
		hdr->last_ade = atoi(tree[FONT_LAST_ADE].ob_spec.tedinfo->te_ptext);
		wind_set_str(mainwin, WF_NAME, fontname);
	}
}

/* -------------------------------------------------------------------------- */

static void do_about(void)
{
	OBJECT *tree = rs_tree(ABOUT_DIALOG);
	tree[ABOUT_VERSION].ob_spec.free_string = PACKAGE_VERSION;
	tree[ABOUT_DATE].ob_spec.free_string = PACKAGE_DATE;
	do_dialog(ABOUT_DIALOG);
}

/* -------------------------------------------------------------------------- */

static void do_help(void)
{
	do_dialog(HELP_DIALOG);
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static void msg_mn_select(_WORD title, _WORD entry)
{
	_WORD message[8];
	
	message[0] = MN_SELECTED;
	message[1] = gl_apid;
	message[2] = 0;
	message[3] = title;
	message[4] = entry;
	message[5] = message[6] = message[7] = 0;
	appl_write(gl_apid, 16, message);
}

/* -------------------------------------------------------------------------- */

static void char_prev(void)
{
	if (cur_char <= fonthdr.first_ade)
		cur_char = fonthdr.last_ade;
	else
		cur_char = (cur_char - fonthdr.first_ade - 1u) % numoffs + fonthdr.first_ade;
	maybe_resize_window();
	redraw_win(mainwin);
}

/* -------------------------------------------------------------------------- */

static void char_next(void)
{
	if (cur_char >= fonthdr.last_ade)
		cur_char = fonthdr.first_ade;
	else
		cur_char = (cur_char - fonthdr.first_ade + 1u) % numoffs + fonthdr.first_ade;
	maybe_resize_window();
	redraw_win(mainwin);
}

/* -------------------------------------------------------------------------- */

static void char_first(void)
{
	cur_char = fonthdr.first_ade;
	maybe_resize_window();
	redraw_win(mainwin);
}

/* -------------------------------------------------------------------------- */

static void char_last(void)
{
	cur_char = fonthdr.last_ade;
	maybe_resize_window();
	redraw_win(mainwin);
}

/* -------------------------------------------------------------------------- */

static void handle_message(_WORD *message, _WORD mox, _WORD moy)
{
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
		if (message[3] == mainwin || message[3] == panelwin)
		{
			wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
		}
		break;

	case WM_TOPPED:
		if (message[3] == mainwin)
		{
			wind_set_int(message[3], WF_TOP, 0);
		} else if (message[3] == panelwin)
		{
			panel_click(mox, moy);
		}
		break;

	case WM_REDRAW:
		wind_update(BEG_UPDATE);
		if (message[3] == mainwin)
			mainwin_draw((const GRECT *)&message[4]);
		else if (message[3] == panelwin)
			panelwin_draw((const GRECT *)&message[4]);
		wind_update(END_UPDATE);
		break;

	case MN_SELECTED:
		switch (message[4])
		{
		case ABOUT:
			do_about();
			break;
		case FOPEN:
			select_font();
			break;
		case FSAVE:
			save_font(fontfilename);
			break;
		case FEXPORTC:
			export_font_c();
			break;
		case FEXPORTTXT:
			export_font_txt();
			break;
		case FIMPORTTXT:
			import_font_txt();
			break;
		case FSYS_6X6:
			font_load_sysfont(0);
			break;
		case FSYS_8X8:
			font_load_sysfont(1);
			break;
		case FSYS_8X16:
			font_load_sysfont(2);
			break;
		case FSYS_16X32:
			font_load_sysfont(3);
			break;
		case FINFO:
			font_info();
			break;
		case FQUIT:
			quit_app = TRUE;
			break;
		case CHAR_NEXT:
			char_next();
			break;
		case CHAR_PREV:
			char_prev();
			break;
		case CHAR_FIRST:
			char_first();
			break;
		case CHAR_LAST:
			char_last();
			break;
		}
		menu_tnormal(menu, message[3], TRUE);
		break;
	}
}

/* -------------------------------------------------------------------------- */

static void mainloop(void)
{
	_WORD event;
	_WORD message[8];
	_WORD k, kstate, dummy, mox, moy;

	if (!is_font_loaded())
		msg_mn_select(TFILE, FOPEN);
	init_linea();
	menu_ienable(menu, FSYS_16X32, Fonts != NULL && Fonts->font[3] != NULL);
	
	while (!quit_app)
	{
		menu_ienable(menu, FINFO, is_font_loaded());
		menu_ienable(menu, FSAVE, is_font_loaded());
		menu_ienable(menu, FEXPORTC, is_font_loaded());
		menu_ienable(menu, FEXPORTTXT, is_font_loaded());
		menu_ienable(menu, CHAR_NEXT, is_font_loaded());
		menu_ienable(menu, CHAR_PREV, is_font_loaded());
		menu_ienable(menu, CHAR_LAST, is_font_loaded());
		menu_ienable(menu, CHAR_FIRST, is_font_loaded());
		
		event = evnt_multi(MU_KEYBD | MU_MESAG | MU_BUTTON,
			256|2, 3, 0,
			0, 0, 0, 0, 0,
			0, 0, 0 ,0, 0,
			message,
			0L,
			&mox, &moy, &dummy, &kstate, &k, &dummy);
		
		if (event & MU_KEYBD)
		{
			switch (k & 0xff)
			{
			case 0x0f: /* Ctrl-O */
				select_font();
				break;
			case 0x13: /* Ctrl-S */
				save_font(fontfilename);
				break;
			case 0x05: /* Ctrl-E */
				export_font_c();
				break;
			case 0x09: /* Ctrl-I */
				font_info();
				break;
			case 0x11: /* Ctrl-Q */
				quit_app = TRUE;
				break;
			case 0x0e: /* Ctrl-N */
				char_next();
				break;
			case 0x10: /* Ctrl-P */
				char_prev();
				break;
			default:
				switch ((k >> 8) & 0xff)
				{
				case 0x48: /* cursor up */
					scalex++;
					scaley++;
					resize_window();
					redraw_win(mainwin);
					break;
				case 0x50: /* cursor down */
					if (scaley > 1)
					{
						scalex--;
						scaley--;
						resize_window();
						redraw_win(mainwin);
					}
					break;
				case 0x4b: /* cursor left */
					char_prev();
					break;
				case 0x4d: /* cursor right */
					char_next();
					break;
				case 0x62: /* Help */
					do_help();
					break;
				default:
					k &= 0xff;
					if (!(kstate & (K_CTRL|K_ALT)) && k >= fonthdr.first_ade && k <= fonthdr.last_ade)
					{
						cur_char = k;
						maybe_resize_window();
						redraw_win(mainwin);
					}
					break;
				}
				break;
			}
		}
		
		if (event & MU_MESAG)
		{
			handle_message(message, mox, moy);
			event &= ~MU_BUTTON;
		}
		
		if (event & MU_BUTTON)
		{
			_WORD win = wind_find(mox, moy);
			if (win == mainwin)
			{
				mainwin_click(mox, moy);
			} else if (win == panelwin)
			{
				panel_click(mox, moy);
			}
		}
		
		if (quit_app && font_changed)
		{
			k = form_alert(1, rs_str(AL_QUIT));
			if (k != 2)
				quit_app = FALSE;
		}
	}
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
	int retval = 0;
	_WORD dummy;
	
	quit_app = FALSE;
	
	app_id = appl_init();
	if (app_id < 0)
	{
		/* fprintf(stderr, "Could not open display\n"); */
		return 1;
	}
	aeshandle = graf_handle(&gl_wchar, &gl_hchar, &dummy, &dummy);
	
	/* just in case; might be unnecessary */
	font_cw = gl_wchar;
	font_ch = gl_hchar;
	
	if (!rsrc_load("gemfedit.rsc"))
	{
		form_alert(1, "[3][Resource file not found][OK]");
		quit_app = TRUE;
		retval = 1;
	}
	
	if (!quit_app)
	{
		if (!open_screen() ||
			!create_panel_window() ||
			!create_window())
		{
			quit_app = TRUE;
			retval = 1;
		}
	}
	
	if (!quit_app)
	{
		menu = rs_tree(MAINMENU);
		menu_bar(menu, TRUE);
		if (argc > 1)
			font_load_gemfont(argv[1]);
#if 1
		else
			font_load_gemfont("..\\fonts\\tos\\system10.fnt");
#endif
	}
	
	graf_mouse(ARROW, NULL);
	
	if (!quit_app)
		mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
