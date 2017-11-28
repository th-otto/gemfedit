#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <ctype.h>
#include <mint/arch/nf_ops.h>
#define FONT_HDR LA_FONT_HDR
#define FONTS LA_FONTS
#define Fonts LA_Fonts
#define fonthdr la_fonthdr
#include <linea.h>
#undef fonthdr
#undef FONT_HDR
#undef FONTS
#undef Fonts
static _WORD gl_wchar, gl_hchar;
#define GetTextSize(w, h) *(w) = gl_wchar, *(h) = gl_hchar
#define hfix_objs(a, b, c)
#define hrelease_objs(a, b)
#include "gemfedit.h"
#include "fonthdr.h"
#include "swap.h"

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

static _WORD font_cw;
static _WORD font_ch;
static FONT_HDR fonthdr;
static unsigned char *fontmem;
static unsigned char *dat_table;
static unsigned short *off_table;
static unsigned char *hor_table;
static char fontname[VDI_FONTNAMESIZE + 1];
static const char *fontfilename;
static const char *fontbasename;
static unsigned short numoffs;
static unsigned short cur_char;
static _BOOL font_changed = FALSE;

#define FONT_BIG ((fonthdr.flags & FONTF_BIGENDIAN) != 0)

LINEA *Linea;
VDIESC *Vdiesc;
FONTS *Fonts;
LINEA_FUNP *Linea_funp;

static char const program_name[] = "gemfedit";


static OBJECT *rs_tree(_WORD num)
{
	OBJECT *tree = NULL;
	rsrc_gaddr(R_TREE, num, &tree);
	return tree;
}

	
static char *rs_str(_WORD num)
{
	char *str = NULL;
	rsrc_gaddr(R_STRING, num, &str);
	return str;
}

	
static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen);
	dst[maxlen - 1] = '\0';
	len = strlen(dst);
	while (len > 0 && dst[len - 1] == ' ')
		dst[--len] = '\0';
}


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





#define ror(x) (((x) >> 1) | ((x) & 1 ? 0x80 : 0))

static _BOOL char_testbit(unsigned short c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (dat_table == NULL)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (_ULONG)y * fonthdr.form_width + (o >> 3);
	return (*dat & mask) != 0;
}


static _BOOL char_togglebit(unsigned short c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (dat_table == NULL)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (_ULONG)y * fonthdr.form_width + (o >> 3);
	*dat ^= mask;
	
	return TRUE;
}


static _BOOL char_setbit(unsigned short c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;

	if (dat_table == NULL)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (_ULONG)y * fonthdr.form_width + (o >> 3);
	if (!(*dat & mask))
	{
		*dat |= mask;
		return TRUE;
	}
	return FALSE;
}


static _BOOL char_clearbit(unsigned short c, _WORD x, _WORD y)
{
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat;
	
	if (dat_table == NULL)
		return FALSE;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;
	if (x < 0 || x >= width || y < 0 || y >= height)
		return FALSE;
	
	o = off_table[c] + x;
	b = o & 0x7;
	mask = 0x80 >> b;
	dat = dat_table + (_ULONG)y * fonthdr.form_width + (o >> 3);
	if (*dat & mask)
	{
		*dat &= ~mask;
		return TRUE;
	}
	return FALSE;
}


static void draw_char(unsigned short c, _WORD x0, _WORD y0)
{
	_WORD x, y;
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat, *p;
	
	if (dat_table == NULL)
		return;
	c -= fonthdr.first_ade;
	width = off_table[c + 1] - off_table[c];
	height = font_ch;

	vsf_color(vdihandle, BLACK);
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


static void mainwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[8];
	_WORD x, y;
	_WORD scaled_w, scaled_h;
	
	v_hide_c(vdihandle);
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	scaled_w = font_cw * scalex + (font_cw + 1) * scaled_margin;
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
			vsf_color(vdihandle, WHITE);
			vsf_perimeter(vdihandle, 0);
			vsf_interior(vdihandle, FIS_SOLID);
			vr_recfl(vdihandle, pxy);
	
			vsf_color(vdihandle, RED);
			for (y = 0; y < (font_ch + 1); y++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN;
				pxy[1] = work.g_y + MAIN_Y_MARGIN + y * (scaley + scaled_margin);
				pxy[2] = pxy[0] + scaled_w - 1;
				pxy[3] = pxy[1] + scaled_margin - 1;
				vr_recfl(vdihandle, pxy);
			}
			for (x = 0; x < (font_cw + 1); x++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN + x * (scalex + scaled_margin);
				pxy[1] = work.g_y + MAIN_Y_MARGIN;
				pxy[2] = pxy[0] + scaled_margin - 1;
				pxy[3] = pxy[1] + scaled_h - 1;
				vr_recfl(vdihandle, pxy);
			}
			
			if (cur_char >= fonthdr.first_ade && cur_char <= fonthdr.last_ade)
				draw_char(cur_char, work.g_x + MAIN_X_MARGIN + scaled_margin, work.g_y + MAIN_Y_MARGIN + scaled_margin);
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}


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
		redraw_win(mainwin);
	}
}


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


static _BOOL open_screen(void)
{
	_WORD work_in[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
	_WORD work_out[57];

	vdihandle = aeshandle;
	(void) v_opnvwk(work_in, &vdihandle, work_out);	/* VDI workstation needed */
	
	return TRUE;
}


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


static void resize_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	_WORD width, height;
	
	width = MAIN_X_MARGIN * 2 + scalex * font_cw + (font_cw + 1) * scaled_margin + 50;
	height = MAIN_Y_MARGIN * 2 + scaley * font_ch + (font_ch + 1) * scaled_margin;
	
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


static _WORD _CDECL draw_font(PARMBLK *pb)
{
	_WORD tattrib[10];
	_WORD fattrib[5];
	_WORD mattrib[5];
	_WORD x, y;
	unsigned short c;
	_WORD pxy[4];
	_WORD dummy;
	_WORD basec;
	
	vqt_attributes(aeshandle, tattrib);
	vqf_attributes(aeshandle, fattrib);
	vqm_attributes(aeshandle, mattrib);
	vsf_color(aeshandle, WHITE);
	vsf_interior(aeshandle, FIS_SOLID);
	vsf_perimeter(aeshandle, FALSE);
	vsm_color(aeshandle, BLACK);
	vsm_type(aeshandle, 1);
	vsm_height(aeshandle, 1);
	pxy[0] = pb->pb_x;
	pxy[1] = pb->pb_y;
	pxy[2] = pb->pb_x + pb->pb_w - 1;
	pxy[3] = pb->pb_y + pb->pb_h - 1;
	vr_recfl(aeshandle, pxy);
	basec = (pb->pb_obj - PANEL_FIRST) * 16;
	for (c = 0; c < 16; c++)
	{
		for (y = 0; y < font_ch; y++)
		{
			for (x = 0; x < font_cw; x++)
			{
				if (char_testbit(basec + c, x, y))
				{
					pxy[0] = pb->pb_x + c * font_cw + x;
					pxy[1] = pb->pb_y + y;
					v_pmarker(aeshandle, 1, pxy);
				}
			}
		}
	}

	vst_color(aeshandle, tattrib[1]);
	vst_rotation(aeshandle, tattrib[2]);
	vst_alignment(aeshandle, tattrib[3], tattrib[4], &dummy, &dummy);
	vst_height(aeshandle, tattrib[7], &dummy, &dummy, &dummy, &dummy);

	vsf_interior(aeshandle, fattrib[0]);
	vsf_color(aeshandle, fattrib[1]);
	vsf_style(aeshandle, fattrib[2]);
	vswr_mode(aeshandle, fattrib[3]);
	vsf_perimeter(aeshandle, fattrib[4]);

	vsm_type(aeshandle, mattrib[0]);
	vsm_color(aeshandle, mattrib[1]);
	vsm_height(aeshandle, mattrib[3]);
	
	return 0;
}

static USERBLK draw_font_userblk = { draw_font, 0 };

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


static void font_gethdr(FONT_HDR *hdr, const unsigned char *h)
{
	hdr->font_id = LOAD_W(h + 0);
	hdr->point = LOAD_W(h + 2);
	chomp(fontname, (const char *)h + 4, VDI_FONTNAMESIZE + 1);
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
}


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


/*
 * There are apparantly several fonts that have the Motorola flag set
 * but are stored in little-endian format.
 */
static _BOOL check_gemfnt_header(FONT_HDR *h, unsigned long l)
{
	UW firstc, lastc, points;
	UW form_width, form_height;
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
	form_width = h->form_width;
	form_height = h->form_height;
	if ((dat_offset + (_ULONG)form_width * form_height) > l)
		return FALSE;
	h->last_ade = lastc;
	return TRUE;
}



static void font_get_tables(unsigned char *h, const char *filename)
{
	FONT_HDR *hdr = &fonthdr;
	unsigned short *u;
	_BOOL hor_table_valid;

	numoffs = hdr->last_ade - hdr->first_ade + 1;
	off_table = (unsigned short *)(h + hdr->off_table);
	dat_table = h + hdr->dat_table;
	hor_table_valid = hdr->hor_table != 0 && hdr->hor_table != hdr->off_table && (hdr->off_table - hdr->hor_table) >= (numoffs * 2);
	if ((hdr->flags & FONTF_HORTABLE) && hor_table_valid)
	{
		hor_table = h + hdr->hor_table;
	} else
	{
		if (hdr->flags & FONTF_HORTABLE)
		{
			nf_debugprintf("%s: warning: %s: flag for horizontal table set, but there is none\n", program_name, filename);
			hdr->flags &= ~FONTF_HORTABLE;
		} else if (hor_table_valid)
		{
			nf_debugprintf("%s: warning: %s: offset table present but flag not set\n", program_name, filename);
		}
		hor_table = NULL;
		hdr->hor_table = 0;
	}
	if (FONT_BIG != HOST_BIG)
	{
		for (u = off_table; u <= off_table + numoffs; u++)
		{
			SWAP_W(*u);
		}
	}
	
	font_cw = hdr->max_cell_width;
	font_ch = hdr->form_height;
	cur_char = 'A';
	
	if (hor_table)
	{
		form_alert(1, rs_str(AL_NO_OFFTABLE));
	}
}



static _BOOL font_gen_gemfont(unsigned char *h, const char *filename, unsigned long l)
{
	FONT_HDR *hdr = &fonthdr;
	
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
				if (HOST_BIG)
				{
					nf_debugprintf("%s: warning: %s: wrong endian flag in header\n", program_name, filename);
					/*
					 * host big-endian, font claims to be big-endian,
					 * but check succeded only after swapping:
					 * font apparently is little-endian, clear flag
					 */
					hdr->flags &= ~FONTF_BIGENDIAN;
				}
			}
		}
	} else
	{
		if (!FONT_BIG)
		{
			if (HOST_BIG)
			{
				nf_debugprintf("%s: warning: %s: wrong endian flag in header\n", program_name, filename);
				/*
				 * host big-endian, font claims to be little-endian,
				 * but check succeded without swapping:
				 * font apparently is big-endian, set flag
				 */
				hdr->flags |= FONTF_BIGENDIAN;
			}
		}
	}
	
	font_get_tables(h, filename);
	
	return TRUE;
}


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
	resize_window();
	resize_panel();
	wind_set_str(mainwin, WF_NAME, fontname);
	fontfilename = filename;
	fontbasename = xbasename(filename);
	wind_set_str(panelwin, WF_NAME, fontbasename);
	redraw_win(mainwin);
	redraw_win(panelwin);
	font_changed = FALSE;
}


static _BOOL font_load_gemfont(const char *filename)
{
	FILE *in;
	unsigned long l;
	unsigned char *h, *m;
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
	m = malloc(l);
	if (m == NULL)
	{
		fclose(in);
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	l = fread(m, 1, l, in);
	h = m;

	ret = font_gen_gemfont(h, filename, l);
	fclose(in);

	if (ret)
	{
		font_loaded(h, filename);
	}

	return ret;
}


#ifdef __PUREC__
static void push_a2(void) 0x2F0A;
static long pop_a2(void) 0x245F;
static void *get_a1(void) 0x2049;
static void *get_a2(void) 0x204A;

static void *linea0(void) 0xa000;

static void init_linea(void)
{
	push_a2();
	Linea = linea0();
	Vdiesc = (VDIESC *)((char *)Linea - sizeof(VDIESC));
	Fonts = get_a1();
	Linea_funp = get_a2();
	pop_a2();
}
#endif


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
	unsigned long offtable_size;
	unsigned long form_size;
	
	if (font_changed)
	{
		if (form_alert(1, rs_str(AL_CHANGED)) != 2)
			return FALSE;
	}
	
	init_linea();
	
	h = (const unsigned char *)(Fonts->font[fontnum]);
	
	font_gethdr(hdr, h);
	numoffs = hdr->last_ade - hdr->first_ade + 1;
	font_get_tables(NULL, filename);
	offtable_size = (unsigned long)(numoffs + 1) * 2;
	form_size = (unsigned long)hdr->form_width * (unsigned long)hdr->form_height;
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
	
	off_table = (unsigned short *)(m + SIZEOF_FONT_HDR);
	dat_table = m + SIZEOF_FONT_HDR + offtable_size;
	
	font_loaded(m, filename);

	return TRUE;
}


static void select_font(void)
{
	_WORD button;
	char filename[128];
	static char path[128];
	char *p;
	
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
	p = xbasename(path);
	strcpy(p, "*.FNT");
	strcpy(filename, "");
	
	if (!fsel_exinput(path, filename, &button, rs_str(SEL_FONT)) || !button)
		return;
	p = xbasename(path);
	strcpy(p, filename);
	font_load_gemfont(path);
}


static _BOOL font_save_gemfont(const char *filename)
{
	FONT_HDR *hdr = &fonthdr;
	FILE *fp;
	unsigned long offtable_size;
	unsigned long form_size;
	unsigned char h[SIZEOF_FONT_HDR + 4];
	unsigned short *u;
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
	
	offtable_size = (unsigned long)(numoffs + 1) * 2;
	form_size = (unsigned long)hdr->form_width * (unsigned long)hdr->form_height;

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

	memcpy(h, hdr, sizeof(h));
	memcpy(h + 4, fontname, VDI_FONTNAMESIZE);
	
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


static void save_font(const char *filename)
{
	_WORD button;
	static char path[128];
	char filename_buf[128];
	char *p;
	
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
	p = xbasename(path);
	strcpy(p, "*.FNT");
	strcpy(filename_buf, "");
	
	if (!fsel_exinput(path, filename_buf, &button, rs_str(SEL_OUTPUT)) || !button)
		return;
	p = xbasename(path);
	strcpy(p, filename_buf);
	font_save_gemfont(path);
}


static void font_info(void)
{
	OBJECT *tree = rs_tree(FONT_PARAMS);
	GRECT gr;
	_WORD ret;
	
	if (dat_table == NULL)
		return;
	form_center_grect(tree, &gr);
	form_dial_grect(FMD_START, &gr, &gr);
	
	strcpy(tree[FONT_NAME].ob_spec.tedinfo->te_ptext, fontname);
	sprintf(tree[FONT_ID].ob_spec.tedinfo->te_ptext, "%5d", fonthdr.font_id);
	sprintf(tree[FONT_POINT].ob_spec.tedinfo->te_ptext, "%5d", fonthdr.point);
	sprintf(tree[FONT_TOP].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.top);
	sprintf(tree[FONT_ASCENT].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.ascent);
	sprintf(tree[FONT_HALF].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.half);
	sprintf(tree[FONT_DESCENT].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.descent);
	sprintf(tree[FONT_BOTTOM].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.descent);
	sprintf(tree[FONT_HEIGHT].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.form_height);
	sprintf(tree[FONT_WIDTH].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.max_cell_width);
	sprintf(tree[FONT_FIRST_ADE].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.first_ade);
	sprintf(tree[FONT_LAST_ADE].ob_spec.tedinfo->te_ptext, "%3d", fonthdr.last_ade);
	
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &gr);
	ret = form_do(tree, ROOT);
	ret &= 0x7fff;
	tree[ret].ob_state &= ~SELECTED;
	
	form_dial_grect(FMD_FINISH, &gr, &gr);
}


static void do_about(void)
{
	OBJECT *tree = rs_tree(ABOUT_DIALOG);
	GRECT gr;
	_WORD ret;

	form_center_grect(tree, &gr);
	form_dial_grect(FMD_START, &gr, &gr);
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &gr);
	ret = form_do(tree, ROOT);
	ret &= 0x7fff;
	tree[ret].ob_state &= ~SELECTED;
	form_dial_grect(FMD_FINISH, &gr, &gr);
}


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


static void handle_message(_WORD *message, _WORD mox, _WORD moy)
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
		if (message[3] == mainwin)
			mainwin_draw((const GRECT *)&message[4]);
		else if (message[3] == panelwin)
			panelwin_draw((const GRECT *)&message[4]);
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
			if (dat_table)
				save_font(fontfilename);
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
		}
		menu_tnormal(menu, message[3], TRUE);
		break;
	}
	wind_update(END_UPDATE);
}


static void mainloop(void)
{
	_WORD event;
	_WORD message[8];
	_WORD k, kstate, dummy, mox, moy;

	if (dat_table == NULL)
		msg_mn_select(TFILE, FOPEN);
	init_linea();
	menu_ienable(menu, FSYS_16X32, Fonts != NULL && Fonts->font[3] != NULL);
	
	while (!quit_app)
	{
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
			case 0x0f:
				select_font();
				break;
			case 0x13:
				if (dat_table)
					save_font(fontfilename);
				break;
			case 0x09:
				font_info();
				break;
			case 0x11:
				quit_app = TRUE;
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
					cur_char = (cur_char - fonthdr.first_ade - 1) % numoffs + fonthdr.first_ade;
					redraw_win(mainwin);
					break;
				case 0x4d: /* cursor right */
					cur_char = (cur_char - fonthdr.first_ade + 1) % numoffs + fonthdr.first_ade;
					redraw_win(mainwin);
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


int main(int argc, char **argv)
{
	int retval = 0;
	_WORD dummy;
	
	quit_app = FALSE;
	
	app_id = appl_init();
	if (app_id < 0)
	{
		fprintf(stderr, "Could not open display\n");
		return 1;
	}
	aeshandle = graf_handle(&gl_wchar, &gl_hchar, &dummy, &dummy);
	
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
		else
			font_load_gemfont("system2.fnt");
	}
	
	graf_mouse(ARROW, NULL);
	
	if (!quit_app)
		mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
