#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <mint/arch/nf_ops.h>
#include <stdint.h>
#ifdef __PUREC__
#include <ext.h>
#else
#include <sys/stat.h>
#endif
#include "spdview.h"
#define RSC_NAMED_FUNCTIONS 1
static _WORD gl_wchar, gl_hchar;
#define GetTextSize(w, h) *(w) = gl_wchar, *(h) = gl_hchar
#define hfix_objs(a, b, c)
#define hrelease_objs(a, b)
#include "spdview.rsh"
#include "speedo.h"
#include "version.h"

#ifndef K_SHIFT
#define K_SHIFT			(K_LSHIFT|K_RSHIFT)
#endif

#undef SWAP_W
#undef SWAP_L
#define SWAP_W(s) s = cpu_swab16(s)
#define SWAP_L(s) s = cpu_swab32(s)

#define PREVIEW_X_MARGIN 10
#define PREVIEW_Y_MARGIN 10
#define PREVIEW_WKIND (NAME | CLOSER | MOVER)

#define MAIN_X_MARGIN 10
#define MAIN_Y_MARGIN 10
#define MAIN_WKIND (NAME | CLOSER | MOVER | SIZER | UPARROW | DNARROW | VSLIDE | LFARROW | RTARROW | HSLIDE)

static _WORD app_id = -1;
static _WORD mainwin = -1;
static _WORD previewwin = -1;
static _WORD aeshandle;
static _WORD vdihandle = 0;
static MFDB screen_fdb;
static _WORD workout[57];
static _WORD xworkout[57];
static _BOOL quit_app;
static OBJECT *menu;
static const int mainwin_gridsize = 1;
static const int preview_gridsize = 1;
static _WORD scalex = 5;
static _WORD scaley = 5;
static _WORD small_cw, small_ch;

static _WORD font_cw;
static _WORD font_ch;
static glyphinfo_t font_bb;

static const char *font_filename;
static buff_t font;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static ufix16 mincharsize;
static char fontname[70 + 1];
static uint16_t first_char_index;
static uint16_t num_chars;
static uint16_t num_ids;
static long char_rows;
static long top_row;
static long left_col;
static _WORD row_height = 1;
static _WORD col_width = 1;
static uint16_t cur_char = 0x41;

static long point_size = 240;
static int x_res = 95;
static int y_res = 95;
static int quality = 1;
static specs_t specs;
static ufix16 char_index, char_id;
#define	MAX_BITS	1024
static _UWORD framebuffer[MAX_BITS][MAX_BITS >> 4];

#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

#define is_font_loaded() (font_buffer != NULL)

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	glyphinfo_t bbox;
	uint16_t *bitmap;
} charinfo;
static charinfo *infos;

#define g_malloc(n) malloc(n)
#define g_calloc(n, s) calloc(n, s)
#define g_malloc0(n) calloc(n, 1)
#define g_realloc(ptr, s) realloc(ptr, s)
#define g_free free

#undef g_new
#define g_new(t, n) ((t *)g_malloc(sizeof(t) * (n)))
#undef g_renew
#define g_renew(t, p, n) ((t *)g_realloc(p, sizeof(t) * (n)))
#undef g_new0
#define g_new0(t, n) ((t *)g_calloc((n), sizeof(t)))

#define DEBUG 0


/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static OBJECT *rs_tree(_WORD num)
{
	return rs_trindex[num];
}

/* ------------------------------------------------------------------------- */
	
static char *rs_str(_WORD num)
{
	return rs_frstr[num];
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/
	
static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen);
	dst[maxlen - 1] = '\0';
	len = strlen(dst);
	while (len > 0 && dst[len - 1] == ' ')
		dst[--len] = '\0';
	while (len > 0 && dst[0] == ' ')
	{
		memmove(dst, dst + 1, len);
		len--;
	}
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

/* ------------------------------------------------------------------------- */

static void cleanup(void)
{
	if (app_id >= 0)
	{
		menu_bar(menu, FALSE);
		appl_exit();
		app_id = -1;
	}
}

/* ------------------------------------------------------------------------- */

static void destroy_win(void)
{
	if (mainwin > 0)
	{
		wind_close(mainwin);
		wind_delete(mainwin);
		mainwin = -1;
	}
	if (previewwin > 0)
	{
		wind_close(previewwin);
		wind_delete(previewwin);
		previewwin = -1;
	}
	if (vdihandle > 0)
	{
		v_clsvwk(vdihandle);
		vdihandle = 0;
	}
}

/* ------------------------------------------------------------------------- */

#define ror(x) (((x) >> 1) | ((x) & 1 ? 0x80 : 0))
#define rorw(x) (((x) >> 1) | ((x) & 1 ? 0x8000 : 0))

static void draw_char(uint16_t ch, _WORD x0, _WORD y0)
{
	_WORD pxy[8];
	_WORD colors[2];
	charinfo *c;
	MFDB src;
	char buf[20];
	
	if (!is_font_loaded())
		return;
	if (ch >= num_ids)
		return;
	c = &infos[ch];
	if (c->char_id == UNDEFINED)
		return;
	/*
	 * draw the label
	 */
	sprintf(buf, "%04x", ch);
	vst_color(vdihandle, G_BLACK);
	vswr_mode(vdihandle, MD_TRANS);
	v_gtext(vdihandle, x0 + (col_width - 4 * small_cw) / 2, y0, buf);
	/*
	 * draw the char
	 */
	if (c->bitmap == NULL || c->bbox.width == 0 || c->bbox.height == 0)
		return;
	y0 += small_ch + mainwin_gridsize;
	src.fd_addr = c->bitmap;
	src.fd_wdwidth = (c->bbox.width + 15) >> 4;
	src.fd_w = c->bbox.width;
	src.fd_h = c->bbox.height;
	src.fd_nplanes = 1;
	src.fd_stand = 1;
	src.fd_r1 = src.fd_r2 = src.fd_r3 = 0;
	colors[0] = G_BLACK;
	colors[1] = G_WHITE;
	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = c->bbox.width - 1;
	pxy[3] = c->bbox.height - 1;
	pxy[4] = x0 - (font_bb.lbearing - c->bbox.lbearing);
	pxy[5] = y0 + font_bb.ascent - c->bbox.ascent;
	pxy[6] = pxy[4] + pxy[2];
	pxy[7] = pxy[5] + pxy[3];
	if (screen_fdb.fd_nplanes == 1)
		vro_cpyfm(vdihandle, S_OR_D, pxy, &src, &screen_fdb);
	else
		vrt_cpyfm(vdihandle, MD_TRANS, pxy, &src, &screen_fdb, colors);
}

/* ------------------------------------------------------------------------- */

static void mainwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[8];
	_WORD x, y;
	long scaled_w;
	long scaled_h;
	
	v_hide_c(vdihandle);
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	scaled_w = col_width * (CHAR_COLUMNS - left_col) + mainwin_gridsize;
	scaled_h = row_height * (char_rows - top_row) + mainwin_gridsize;
	if (scaled_w > work.g_w)
		scaled_w = work.g_w;
	if (scaled_h > work.g_h)
		scaled_h = work.g_h;
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
			
			if (mainwin_gridsize > 0)
			{
				vsf_color(vdihandle, G_RED);
				/*
				 * draw the horizontal grid lines
				 */
				for (y = 0; y < (char_rows + 1); y++)
				{
					long yy = MAIN_Y_MARGIN + (y - top_row) * row_height;
					if ((yy + row_height) < 0)
						continue;
					if (yy >= work.g_h)
						break;
					pxy[0] = work.g_x + MAIN_X_MARGIN;
					pxy[1] = work.g_y + (_WORD)yy;
					pxy[2] = pxy[0] + (_WORD)scaled_w - 1;
					pxy[3] = pxy[1] + mainwin_gridsize - 1;
					vr_recfl(vdihandle, pxy);
					pxy[1] += small_ch + mainwin_gridsize;
					pxy[3] += small_ch + mainwin_gridsize;
					vr_recfl(vdihandle, pxy);
				}
				/*
				 * draw the vertical grid lines
				 */
				for (x = 0; x < (CHAR_COLUMNS + 1); x++)
				{
					long xx = MAIN_X_MARGIN + (x - left_col) * col_width;
					if ((xx + col_width) < 0)
						continue;
					if (xx >= work.g_w)
						break;
					pxy[0] = work.g_x + (_WORD)xx;
					pxy[1] = work.g_y + MAIN_Y_MARGIN;
					pxy[2] = pxy[0] + mainwin_gridsize - 1;
					pxy[3] = pxy[1] + (_WORD)scaled_h - 1;
					vr_recfl(vdihandle, pxy);
				}
			}
			
			for (y = 0; y < char_rows; y++)
			{
				long yy = MAIN_Y_MARGIN + (y - top_row) * row_height + mainwin_gridsize;
				if ((yy + row_height) <= (MAIN_Y_MARGIN + mainwin_gridsize))
					continue;
				if (yy >= work.g_h)
					break;
				for (x = 0; x < CHAR_COLUMNS; x++)
				{
					uint16_t c = y * CHAR_COLUMNS + x;
					long xx = MAIN_X_MARGIN + (x - left_col) * col_width + mainwin_gridsize;
					if ((xx + col_width) <= (MAIN_X_MARGIN + mainwin_gridsize))
						continue;
					if (xx >= work.g_w)
						break;
					draw_char(c, work.g_x + (_WORD)xx, work.g_y + (_WORD)yy);
				}
			}
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}

/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */

static void mainwin_click(_WORD mox, _WORD moy)
{
	long x, y;
	GRECT work;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	x = (mox - work.g_x - MAIN_X_MARGIN) / col_width + left_col;
	y = (moy - work.g_y - MAIN_Y_MARGIN) / row_height + top_row;
	if (x >= 0 && x < CHAR_COLUMNS && y >= 0 && y < char_rows)
	{
		cur_char = (uint16_t)(y * CHAR_COLUMNS + x);
	} else
	{
		cur_char = UNDEFINED;
	}
	redraw_win(previewwin);
}

/* ------------------------------------------------------------------------- */

static void preview_line(const GRECT *work, _WORD x, _WORD y, _WORD w, _WORD h)
{
	_WORD pxy[4];
	
	pxy[0] = work->g_x + PREVIEW_X_MARGIN + x * (scalex + preview_gridsize);
	pxy[1] = work->g_y + PREVIEW_Y_MARGIN + y * (scaley + preview_gridsize);
	pxy[2] = pxy[0] + w * (scalex + preview_gridsize) + preview_gridsize - 1;
	pxy[3] = pxy[1] + h * (scaley + preview_gridsize) + preview_gridsize - 1;
	vr_recfl(vdihandle, pxy);
}

/* -------------------------------------------------------------------------- */

static void draw_preview(const GRECT *work, uint16_t ch)
{
	charinfo *c;
	uint16_t *dat, *p;
	_WORD x0, y0;
	_WORD x, y;
	_UWORD mask;
	_UWORD wdwidth;
	
	if (!is_font_loaded())
		return;
	if (ch >= num_ids)
		return;
	c = &infos[ch];
	if (c->bitmap == NULL || c->bbox.width == 0 || c->bbox.height == 0)
		return;
	x0 = work->g_x + PREVIEW_X_MARGIN + preview_gridsize;
	y0 = work->g_y + PREVIEW_Y_MARGIN + preview_gridsize;
	x0 -= (font_bb.lbearing - c->bbox.lbearing) * (scalex + preview_gridsize);
	y0 += (font_bb.ascent - c->bbox.ascent) * (scaley + preview_gridsize);
	vsf_color(vdihandle, G_BLACK);
	dat = c->bitmap;
	wdwidth = (c->bbox.width + 15) >> 4;
	for (y = 0; y < c->bbox.height; y++, dat += wdwidth)
	{
		mask = 0x8000;
		p = dat;
		for (x = 0; x < c->bbox.width; x++)
		{
			if (*p & mask)
			{
				_WORD pxy[4];
				
				pxy[0] = x0 + x * (scalex + preview_gridsize);
				pxy[1] = y0 + y * (scaley + preview_gridsize);
				pxy[2] = pxy[0] + scalex - 1;
				pxy[3] = pxy[1] + scaley - 1;
				vr_recfl(vdihandle, pxy);
			}
			mask = rorw(mask);
			if (mask == 0x8000)
				p++;
		}
	}

	if (preview_gridsize > 0)
	{
		/*
		 * draw the bounding box
		 */
		vsf_color(vdihandle, screen_fdb.fd_nplanes == 1 ? G_WHITE : G_GREEN);
		x0 = -(font_bb.lbearing - c->bbox.lbearing);
		y0 = (font_bb.ascent - c->bbox.ascent);
		preview_line(work, x0, y0, c->bbox.width, 0);
		preview_line(work, x0, y0, 0, c->bbox.height);
		preview_line(work, x0, y0 + c->bbox.height, c->bbox.width, 0);
		preview_line(work, x0 + c->bbox.width, y0, 0, c->bbox.height);
		/*
		 * draw the baseline
		 */
		vsf_color(vdihandle, screen_fdb.fd_nplanes == 1 ? G_WHITE : G_BLUE);
		y0 = font_bb.ascent;
		preview_line(work, 0, y0, font_cw, 0);
	}
}

/* ------------------------------------------------------------------------- */

static void previewwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[4];
	_WORD x, y;
	
	wind_get_grect(previewwin, WF_WORKXYWH, &work);
	v_hide_c(vdihandle);
	wind_get_grect(previewwin, WF_FIRSTXYWH, &gr);
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

			if (preview_gridsize > 0)
			{
				vsf_color(vdihandle, G_RED);
				/*
				 * draw the horizontal grid lines
				 */
				for (y = 0; y < (font_ch + 1); y++)
				{
					preview_line(&work, 0, y, font_cw, 0);
				}
				/*
				 * draw the vertical grid lines
				 */
				for (x = 0; x < (font_cw + 1); x++)
				{
					preview_line(&work, x, 0, 0, font_ch);
				}
			}
			draw_preview(&work, cur_char);
		}
		wind_get_grect(previewwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}

/* ------------------------------------------------------------------------- */

static void preview_click(_WORD mox, _WORD moy)
{
	UNUSED(mox);
	UNUSED(moy);
	/* nothing to do for now */
}

/* ------------------------------------------------------------------------- */

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
	screen_fdb.fd_wdwidth = (screen_fdb.fd_w + 15) >> 4;
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
	
	/*
	 * get size of small font
	 */
	vst_height(vdihandle, 5, &dummy, &dummy, &small_cw, &small_ch);
	
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void do_alert(const char *str)
{
	graf_mouse(ARROW, NULL);
	form_alert(1, str);
}

/* ------------------------------------------------------------------------- */

static void do_alert1(_WORD num)
{
	do_alert(rs_str(num));
}

/* ------------------------------------------------------------------------- */

static _BOOL create_preview_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	
	/* create a window */
	gr.g_x = 100;
	gr.g_y = 100;
	gr.g_w = PREVIEW_X_MARGIN * 2 + (scalex + preview_gridsize) * font_cw + preview_gridsize;
	gr.g_h = PREVIEW_Y_MARGIN * 2 + (scaley + preview_gridsize) * font_ch + preview_gridsize;
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = PREVIEW_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	gr.g_x = 20;
	gr.g_y = 20;
	rc_intersect(&desk, &gr);
	previewwin = wind_create_grect(wkind, &desk);
	if (previewwin < 0)
	{
		do_alert1(AL_NOWINDOW);
		return FALSE;
	}

	wind_set_str(previewwin, WF_NAME, "");
	wind_open_grect(previewwin, &gr);
	
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void calc_vslider(void)
{
	GRECT gr;
	long max_doc;
	long hh;
	long val;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	hh = row_height * char_rows;
	max_doc = hh - gr.g_h;
	if (hh <= 0)
	{
		val = 1000;
	} else
	{
		val = (1000l * gr.g_h) / hh;
		if (val > 1000)
			val = 1000;
	}
	wind_set_int(mainwin, WF_VSLSIZE, (_WORD)val);
	if (max_doc <= 0)
	{
		val = 0;
	} else
	{
		long yy = top_row * row_height;
		val = (1000l * yy) / max_doc;
	}
	wind_set_int(mainwin, WF_VSLIDE, (_WORD)val);
}

/* ------------------------------------------------------------------------- */

static void calc_hslider(void)
{
	GRECT gr;
	long max_doc;
	long ww;
	long val;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	ww = col_width * CHAR_COLUMNS;
	max_doc = ww - gr.g_w;
	if (ww <= 0)
	{
		val = 1000;
	} else
	{
		val = (1000l * gr.g_w) / ww;
		if (val > 1000)
			val = 1000;
	}
	wind_set_int(mainwin, WF_HSLSIZE, (_WORD)val);
	if (max_doc <= 0)
	{
		val = 0;
	} else
	{
		long xx = left_col * col_width;
		val = (1000l * xx) / max_doc;
	}
	wind_set_int(mainwin, WF_HSLIDE, (_WORD)val);
}

/* ------------------------------------------------------------------------- */

static _BOOL create_window(void)
{
	GRECT gr, desk, preview;
	_WORD wkind;
	_WORD width;
	long height;
	
	wind_get_grect(DESK, WF_WORKXYWH, &desk);

	width = MAIN_X_MARGIN * 2 + col_width * CHAR_COLUMNS + mainwin_gridsize;
	height = MAIN_Y_MARGIN * 2 + row_height * char_rows + mainwin_gridsize;
	if (height > desk.g_h)
		height = desk.g_h;
	
	/* create a window */
	gr.g_x = 100;
	gr.g_y = 100;
	gr.g_w = width;
	gr.g_h = (_WORD)height;
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	wind_get_grect(previewwin, WF_CURRXYWH, &preview);
	gr.g_x = preview.g_x + preview.g_w + 20;
	gr.g_y = preview.g_y;
	rc_intersect(&desk, &gr);
	mainwin = wind_create_grect(wkind, &desk);
	if (mainwin < 0)
	{
		do_alert1(AL_NOWINDOW);
		return FALSE;
	}

	wind_set_str(mainwin, WF_NAME, "");
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	calc_vslider();
	calc_hslider();
	wind_open_grect(mainwin, &gr);
	
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static _BOOL resize_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	_WORD width;
	long height;
	
	wind_get_grect(DESK, WF_WORKXYWH, &desk);

	wind_get_grect(previewwin, WF_CURRXYWH, &gr);
	wkind = PREVIEW_WKIND;
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	gr.g_w = PREVIEW_X_MARGIN * 2 + (scalex + preview_gridsize) * font_cw + preview_gridsize;
	gr.g_h = PREVIEW_Y_MARGIN * 2 + (scaley + preview_gridsize) * font_ch + preview_gridsize;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	wind_set_grect(previewwin, WF_CURRXYWH, &gr);
	
	width = MAIN_X_MARGIN * 2 + col_width * CHAR_COLUMNS + mainwin_gridsize;
	height = MAIN_Y_MARGIN * 2 + row_height * char_rows + mainwin_gridsize;
	if (height > desk.g_h)
		height = desk.g_h;
	
	wind_get_grect(mainwin, WF_CURRXYWH, &gr);
	wkind = MAIN_WKIND;
	wind_get_grect(previewwin, WF_CURRXYWH, &gr);
	gr.g_x = gr.g_x + gr.g_w + 20;
	gr.g_y = gr.g_y;
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	gr.g_w = width;
	gr.g_h = (_WORD)height;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	wind_set_grect(mainwin, WF_CURRXYWH, &gr);
	calc_vslider();
	calc_hslider();
	
	return TRUE;
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static FILE *fp;
static fix15 bit_width, bit_height;

/*
 * Reads 1-byte field from font buffer 
 */
#define read_1b(pointer) (*(pointer))

/* ------------------------------------------------------------------------- */

static fix15 read_2b(ufix8 *ptr)
{
	fix15 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

static fix31 read_4b(ufix8 *ptr)
{
	fix31 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

/*
 * Called by Speedo character generator to report an error.
 */
void sp_write_error(const char *str, ...)
{
	va_list v;
	char buf[256];
	char msg[256];
	
	va_start(v, str);
	vsprintf(buf, str, v);
	va_end(v);
	sprintf(msg, "[1][%s][OK]", buf);
	do_alert(msg);
}

/* ------------------------------------------------------------------------- */

void sp_open_bitmap(fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	fix31 width, pix_width;
	bbox_t bb;
	charinfo *c;
	
	UNUSED(xorg);
	UNUSED(yorg);
	bit_width = xsize;
	bit_height = ysize;

	if (bit_width > MAX_BITS)
	{
		nf_debugprintf("char 0x%x (0x%x) wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	width = sp_get_char_width(char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 7200L) / (point_size * y_res);

	sp_get_char_bbox(char_index, &bb, TRUE);

	c = &infos[char_id];
	bit_width = c->bbox.width;
	bit_height = c->bbox.height;

	/* XXX kludge to handle space */
	if (bb.xmin == 0 && bb.ymin == 0 && bb.xmax == 0 && bb.ymax == 0 && width)
	{
		bit_width = 1;
		bit_height = 1;
	}

	if (bit_width > MAX_BITS)
	{
		nf_debugprintf("char 0x%x (0x%x): width too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		nf_debugprintf("char 0x%x (0x%x): height too large /%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_height = MAX_BITS;
	}
	
	memset(framebuffer, 0, sizeof(framebuffer));
}

/* ------------------------------------------------------------------------- */

static int trunc = 0;

void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 < 0 || xbit1 >= bit_width)
	{
		nf_debugprintf("char 0x%x (0x%x): bit1 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit1, bit_width);
		xbit1 = bit_width;
		trunc = 1;
	}
	if (xbit2 < 0 || xbit2 > bit_width)
	{
		nf_debugprintf("char 0x%x (0x%x): bit2 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit2, bit_width);
		xbit2 = bit_width;
		trunc = 1;
	}

	if (y < 0 || y >= bit_height)
	{
		nf_debugprintf("char 0x%x (0x%x): y value %d is larger than height %u -- truncated\n", char_index, char_id, y, bit_height);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		unsigned int x = i >> 4;
		_UWORD mask = 0x8000 >> (i & 0x000f);
		framebuffer[y][x] |= mask;
	}
}

/* ------------------------------------------------------------------------- */

#if INCL_LCD
boolean sp_load_char_data(long file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		nf_debugprintf("%x (%x) can't seek to char at %ld\n", char_index, char_id, file_offset);
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		nf_debugprintf("char buf overflow\n");
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		nf_debugprintf("can't get char data\n");
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	char_data->org = c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif

/* ------------------------------------------------------------------------- */

void sp_close_bitmap(void)
{
	charinfo *c = &infos[char_id];
	fix15 y;
	uint16_t *bitmap;
	
	if (c->bbox.width != 0 && c->bbox.height != 0)
	{
		_UWORD wdwidth = (c->bbox.width + 15) >> 4;
		size_t words = (size_t)wdwidth * c->bbox.height;
		bitmap = g_new(uint16_t, words);
		c->bitmap = bitmap;
		if (bitmap != NULL)
		{
			for (y = 0; y < c->bbox.height; y++)
			{
				memcpy(bitmap, &framebuffer[y][0], wdwidth << 1);
				bitmap += wdwidth;
			}
		}
	}
	trunc = 0;
	memset(framebuffer, 0, sizeof(framebuffer));
}

/* ------------------------------------------------------------------------- */

/* outline stubs */
#if INCL_OUTLINE
void sp_open_outline(fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	UNUSED(x_set_width);
	UNUSED(y_set_width);
	UNUSED(xmin);
	UNUSED(xmax);
	UNUSED(ymin);
	UNUSED(ymax);
}

/* ------------------------------------------------------------------------- */

void sp_start_sub_char(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_end_sub_char(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	UNUSED(x);
	UNUSED(y);
	UNUSED(outside);
}

/* ------------------------------------------------------------------------- */

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	UNUSED(x1);
	UNUSED(y1);
	UNUSED(x2);
	UNUSED(y2);
	UNUSED(x3);
	UNUSED(y3);
}

/* ------------------------------------------------------------------------- */

void sp_line_to(fix31 x1, fix31 y1)
{
	UNUSED(x1);
	UNUSED(y1);
}

/* ------------------------------------------------------------------------- */

void sp_close_contour(void)
{
}

/* ------------------------------------------------------------------------- */

void sp_close_outline(void)
{
}
#endif

/* ------------------------------------------------------------------------- */

static void update_bbox(charinfo *c, glyphinfo_t *box)
{
	bbox_t bb;
	
	sp_get_char_bbox(c->char_index, &bb, TRUE);
	c->bbox.xmin = bb.xmin;
	c->bbox.ymin = bb.ymin;
	c->bbox.xmax = bb.xmax;
	c->bbox.ymax = bb.ymax;
	box->xmin = MIN(box->xmin, c->bbox.xmin);
	box->ymin = MIN(box->ymin, c->bbox.ymin);
	box->xmax = MIN(box->xmax, c->bbox.xmax);
	box->ymax = MIN(box->ymax, c->bbox.ymax);
	c->bbox.width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
	c->bbox.height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
	c->bbox.lbearing = (bb.xmin + 32768L) >> 16;
	c->bbox.off_vert = bb.ymin >> 16;
	box->lbearing = MIN(box->lbearing, c->bbox.lbearing);
	c->bbox.rbearing = c->bbox.width + c->bbox.lbearing;
	box->rbearing = MAX(box->rbearing, c->bbox.rbearing);
	c->bbox.ascent = c->bbox.height + c->bbox.off_vert;
	c->bbox.descent = c->bbox.height - c->bbox.ascent;
	box->width = MAX(box->width, c->bbox.width);
	box->height = MAX(box->height, c->bbox.height);
	box->ascent = MAX(box->ascent, c->bbox.ascent);
	box->descent = MAX(box->descent, c->bbox.descent);
#if 0
	nf_debugprintf("bbox(%x): %d %d %d %d %7.2f %7.2f %7.2f %7.2f\n",
		c->char_id,
		c->bbox.width, c->bbox.height,
		c->bbox.lbearing, -c->bbox.descent,
		(double)c->bbox.xmin / 65536.0,
		(double)c->bbox.ymin / 65536.0,
		(double)c->bbox.xmax / 65536.0,
		(double)c->bbox.ymax / 65536.0);
#endif
}

/* ------------------------------------------------------------------------- */

static _BOOL font_gen_speedo_font(void)
{
	_BOOL decode_ok = TRUE;
	const ufix8 *key;
	uint16_t i;

	/* init */
	sp_reset();

	key = sp_get_key(&font);
	if (key == NULL)
	{
		sp_write_error("Non-standard encryption");
		return FALSE;
	} else
	{
		sp_set_key(key);
	}
	
	first_char_index = read_2b(font_buffer + FH_FCHRF);
	num_chars = read_2b(font_buffer + FH_NCHRL);
	if (num_chars == 0)
	{
		do_alert1(AL_NO_GLYPHS);
		return FALSE;
	} 
	
	/* set up specs */
	/* Note that point size is in decipoints */
	specs.xxmult = point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1L << 16);
	specs.yoffset = 0L << 16;
	switch (quality)
	{
	case 0:
		specs.flags = MODE_BLACK;
		break;
	case 1:
		specs.flags = MODE_SCREEN;
		break;
	case 2:
#if INCL_OUTLINE
		specs.flags = MODE_OUTLINE;
#else
		specs.flags = MODE_2D;
#endif
		break;
	case 3:
		specs.flags = MODE_2D;
		break;
	}

	chomp(fontname, (char *) (font_buffer + FH_FNTNM), sizeof(fontname));
	
	if (!sp_set_specs(&specs, &font))
	{
		decode_ok = FALSE;
	} else
	{
		num_ids = 0;
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != SP_UNDEFINED && char_id != UNDEFINED && char_id >= num_ids)
				num_ids = char_id + 1;
		}
		
		if (num_ids == 0)
		{
			do_alert1(AL_NO_GLYPHS);
			return FALSE;
		}
		char_rows = (num_ids + CHAR_COLUMNS - 1) / CHAR_COLUMNS;
		
		infos = g_new(charinfo, num_ids);
		if (infos == NULL)
		{
			do_alert1(AL_NOMEM);
			return FALSE;
		}
		
		for (i = 0; i < num_ids; i++)
		{
			infos[i].char_index = 0;
			infos[i].char_id = UNDEFINED;
			infos[i].bitmap = NULL;
		}

		/*
		 * determine the average width of all the glyphs in the font
		 */
		{
			unsigned long total_width;
			uint16_t num_glyphs;
			glyphinfo_t max_bb;
			fix15 max_lbearing;
			fix15 min_descent;
			fix15 average_width;
			
			total_width = 0;
			num_glyphs = 0;
			max_bb.width = 0;
			max_bb.height = 0;
			max_bb.off_vert = 0;
			max_bb.ascent = -32000;
			max_bb.descent = -32000;
			max_bb.rbearing = -32000;
			max_bb.lbearing = 32000;
			max_bb.xmin = 32000L << 16;
			max_bb.ymin = 32000L << 16;
			max_bb.xmax = -(32000L << 16);
			max_bb.ymax = -(32000L << 16);
			max_lbearing = -32000;
			min_descent = 32000;
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(char_index);
				if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
				{
					charinfo *c = &infos[char_id];
					
					if (c->char_id != UNDEFINED)
					{
						nf_debugprintf("char 0x%x (0x%x) already defined\n", char_index, char_id);
					} else
					{
						c->char_index = char_index;
						c->char_id = char_id;
						update_bbox(c, &max_bb);
						max_lbearing = MAX(max_lbearing, c->bbox.lbearing);
						min_descent = MIN(min_descent, c->bbox.descent);
						total_width += c->bbox.width;
						num_glyphs++;
					}
				}
			}
			if (num_glyphs == 0)
				average_width = max_bb.width;
			else
				average_width = (fix15)(total_width / num_glyphs);
			nf_debugprintf("max height %d max width %d average width %d\n", max_bb.height, max_bb.width, average_width);

			max_bb.height = max_bb.ascent + max_bb.descent;
			max_bb.width = max_bb.rbearing - max_bb.lbearing;

			nf_debugprintf("bbox: %d %d ascent %d descent %d lb %d rb %d\n",
				max_bb.width, max_bb.height,
				max_bb.ascent, max_bb.descent,
				max_bb.lbearing, max_bb.rbearing);
			{
				fix15 xmin, ymin, xmax, ymax;
				long fwidth, fheight;
				long pixel_size = (point_size * x_res + 360) / 720;
				
				xmin = read_2b(font_buffer + FH_FXMIN);
				ymin = read_2b(font_buffer + FH_FYMIN);
				xmax = read_2b(font_buffer + FH_FXMAX);
				ymax = read_2b(font_buffer + FH_FYMAX);
				fwidth = xmax - xmin;
				fwidth = fwidth * pixel_size / sp_globals.orus_per_em;
				fheight = ymax - ymin;
				fheight = fheight * pixel_size / sp_globals.orus_per_em;
				nf_debugprintf("bbox (header): %d %d %d %d; %ld %ld %ld %ld\n",
					xmin, ymin,
					xmax, ymax,
					xmin * pixel_size / sp_globals.orus_per_em,
					ymin * pixel_size / sp_globals.orus_per_em,
					xmax * pixel_size / sp_globals.orus_per_em,
					ymax * pixel_size / sp_globals.orus_per_em);
			}
			
			font_ch = max_bb.height;
			font_cw = max_bb.width;
			row_height = small_ch + mainwin_gridsize + font_ch + mainwin_gridsize;
			col_width = font_cw;
			if (col_width < (4 * small_cw))
				col_width = 4 * small_cw;
			col_width += mainwin_gridsize;
			font_bb = max_bb;
		}
		
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
			{
				if (!sp_make_char(char_index))
				{
					nf_debugprintf("can't make char %d (%x)\n", char_index, char_id);
				}
			}
		}
	}

	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static _BOOL font_load_speedo_font(const char *filename)
{
	_BOOL ret;
	ufix8 tmp[FH_FBFSZ + 4];
	ufix32 minbufsize;
	
	graf_mouse(BUSY_BEE, NULL);
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FOPEN), filename);
		do_alert(buf);
		return FALSE;
	}
	if (fread(tmp, sizeof(tmp), 1, fp) != 1)
	{
		fclose(fp);
		do_alert1(AL_READERROR);
		return FALSE;
	}
	if (read_4b(tmp + FH_FMVER + 4) != 0x0d0a0000L)
	{
		fclose(fp);
		sp_report_error(4);
		return FALSE;
	}
	free(font_buffer);
	free(c_buffer);
	c_buffer = NULL;
	minbufsize = read_4b(tmp + FH_FBFSZ);
	font_buffer = g_new(ufix8, minbufsize);
	if (font_buffer == NULL)
	{
		fclose(fp);
		do_alert1(AL_NOMEM);
		return FALSE;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(font_buffer, minbufsize, 1, fp) != 1)
	{
		fclose(fp);
		free(font_buffer);
		font_buffer = NULL;
		do_alert1(AL_READERROR);
		return FALSE;
	}

	mincharsize = read_2b(font_buffer + FH_CBFSZ);

	c_buffer = g_new(ufix8, mincharsize);
	if (c_buffer == NULL)
	{
		fclose(fp);
		free(font_buffer);
		font_buffer = NULL;
		do_alert1(AL_NOMEM);
		return FALSE;
	}

	font.org = font_buffer;
	font.no_bytes = minbufsize;

	ret = font_gen_speedo_font();
	fclose(fp);

	if (ret)
	{
		font_filename = filename;
		font.org = font_buffer;
		top_row = 0;
		left_col = 0;
		resize_window();
		wind_set_str(mainwin, WF_NAME, fontname);
		wind_set_str(previewwin, WF_NAME, xbasename(font_filename));
		redraw_win(mainwin);
		redraw_win(previewwin);
	} else
	{
		font_filename = NULL;
		free(font_buffer);
		font_buffer = NULL;
		free(c_buffer);
		c_buffer = NULL;
		g_free(infos);
		infos = NULL;
		num_ids = 0;
		cur_char = UNDEFINED;
		char_rows = 0;
	}

	graf_mouse(ARROW, NULL);
	return ret;
}

/* ------------------------------------------------------------------------- */

static void set_pointsize(long size)
{
	menu_icheck(menu, POINTS_6, size == 60);
	menu_icheck(menu, POINTS_8, size == 80);
	menu_icheck(menu, POINTS_9, size == 90);
	menu_icheck(menu, POINTS_10, size == 100);
	menu_icheck(menu, POINTS_11, size == 110);
	menu_icheck(menu, POINTS_12, size == 120);
	menu_icheck(menu, POINTS_14, size == 140);
	menu_icheck(menu, POINTS_16, size == 160);
	menu_icheck(menu, POINTS_18, size == 180);
	menu_icheck(menu, POINTS_24, size == 240);
	menu_icheck(menu, POINTS_36, size == 360);
	menu_icheck(menu, POINTS_48, size == 480);
	menu_icheck(menu, POINTS_64, size == 640);
	menu_icheck(menu, POINTS_72, size == 720);

	if (!is_font_loaded() || size == point_size)
		return;
	point_size = size;
	font_load_speedo_font(font_filename);
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static _BOOL do_fsel_input(char *path, char *filename, char *mask, const char *title)
{
	_WORD button = 0;
	_WORD ret;
	char *p;
	
	p = xbasename(path);
	strcpy(p, mask);
	if (gl_ap_version >= 0x0140)
		ret = fsel_exinput(path, filename, &button, title);
	else
		ret = fsel_input(path, filename, &button);
	if (ret == 0 || button == 0)
		return FALSE;
	p = xbasename(path);
	strcpy(mask, p);
	strcpy(p, filename);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void select_font(void)
{
	char filename[128];
	static char path[128];
	static char mask[128] = "*.SPD";
	
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
	font_load_speedo_font(path);
}

/* -------------------------------------------------------------------------- */

static _WORD do_dialog(_WORD num)
{
	OBJECT *tree = rs_tree(num);
	GRECT gr;
	_WORD ret;

	form_center_grect(tree, &gr);
	form_dial_grect(FMD_START, &gr, &gr);
	objc_draw_grect(tree, ROOT, MAX_DEPTH, &gr);
	ret = form_do(tree, ROOT);
	ret &= 0x7fff;
	tree[ret].ob_state &= ~OS_SELECTED;
	form_dial_grect(FMD_FINISH, &gr, &gr);
	return ret;
}

/* ------------------------------------------------------------------------- */

static void set_text(OBJECT *tree, _WORD idx, const void *_text, int maxlen)
{
	TEDINFO *ted;
	char *p;
	const unsigned char *text = (const unsigned char *)_text;
	
	tree += idx;
	ted = tree->ob_spec.tedinfo;
	if (maxlen > (ted->te_txtlen - 1))
		maxlen = ted->te_txtlen - 1;
	p = ted->te_ptext;
	while (--maxlen >= 0)
		*p++ = *text++;
	*p = '\0';
}

/* ------------------------------------------------------------------------- */

static void font_info(void)
{
	OBJECT *tree = rs_tree(FONT_PARAMS);
	char buf[20];
	
	if (!is_font_loaded())
		return;
	set_text(tree, FONT_NAME, font_buffer + FH_FNTNM, 70);
	set_text(tree, FONT_SHORT_NAME, font_buffer + FH_SFNTN, 32);
	set_text(tree, FONT_FACE_NAME, font_buffer + FH_SFACN, 16);
	set_text(tree, FONT_FORM, font_buffer + FH_FNTFM, 14);
	set_text(tree, FONT_DATE, font_buffer + FH_MDATE, 10);
	set_text(tree, FONT_LAYOUT_NAME, font_buffer + FH_LAYNM, 70);
	sprintf(buf, "%5u", read_2b(font_buffer + FH_FNTID));
	set_text(tree, FONT_ID, buf, 5);
	sprintf(buf, "%5u", read_2b(font_buffer + FH_NCHRL));
	set_text(tree, FONT_CHARS_LAYOUT, buf, 5);
	sprintf(buf, "%5u", read_2b(font_buffer + FH_NCHRF));
	set_text(tree, FONT_CHARS_FONT, buf, 5);
	sprintf(buf, "%5u", read_2b(font_buffer + FH_FCHRF));
	set_text(tree, FONT_FIRST_INDEX, buf, 5);
	do_dialog(FONT_PARAMS);
}

/* ------------------------------------------------------------------------- */

static void do_about(void)
{
	OBJECT *tree = rs_tree(ABOUT_DIALOG);

	tree[ABOUT_VERSION].ob_spec.free_string = PACKAGE_VERSION;
	tree[ABOUT_DATE].ob_spec.free_string = PACKAGE_DATE;
	do_dialog(ABOUT_DIALOG);
}

/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */

static void vscroll_to(long yy)
{
	GRECT gr;
	long row;
	long page_h;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	page_h = ((gr.g_h - MAIN_Y_MARGIN) / row_height) * row_height;

	if (yy + page_h > char_rows * row_height)
		yy = char_rows * row_height - page_h;
	if (yy < 0)
		yy = 0;
	row = yy / row_height;
	if (row != top_row)
	{
		top_row = row;
		calc_vslider();
		redraw_win(mainwin);
	}
}

/* ------------------------------------------------------------------------- */

static void hscroll_to(long xx)
{
	GRECT gr;
	long col;
	long page_w;
	
	wind_get_grect(mainwin, WF_WORKXYWH, &gr);
	page_w = ((gr.g_w - MAIN_Y_MARGIN) / col_width) * col_width;

	if (xx + page_w > CHAR_COLUMNS * col_width)
		xx = CHAR_COLUMNS * col_width - page_w;
	if (xx < 0)
		xx = 0;
	col = xx / col_width;
	if (col != left_col)
	{
		left_col = col;
		calc_hslider();
		redraw_win(mainwin);
	}
}

/* ------------------------------------------------------------------------- */

static void handle_message(_WORD *message, _WORD mox, _WORD moy)
{
	UNUSED(mox);
	UNUSED(moy);
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
			wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
			calc_vslider();
			calc_hslider();
		}
		break;

	case WM_MOVED:
		if (message[3] == mainwin || message[3] == previewwin)
		{
			wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
		}
		break;

	case WM_TOPPED:
		if (message[3] == mainwin)
		{
			wind_set_int(message[3], WF_TOP, 0);
		} else if (message[3] == previewwin)
		{
			wind_set_int(message[3], WF_TOP, 0);
		}
		break;

	case WM_REDRAW:
		if (message[3] == mainwin)
			mainwin_draw((const GRECT *)&message[4]);
		else if (message[3] == previewwin)
			previewwin_draw((const GRECT *)&message[4]);
		break;
	
	case WM_ARROWED:
		if (message[3] == mainwin)
		{
			long amount;
			GRECT gr;
			long yy, xx;
			long page_h;
			long page_w;
			
			wind_get_grect(mainwin, WF_WORKXYWH, &gr);

			amount = 0;
			page_h = ((gr.g_h - MAIN_Y_MARGIN) / row_height) * row_height;
			switch (message[4])
			{
			case WA_UPLINE:
				amount = -row_height;
				break;
			case WA_DNLINE:
				amount = row_height;
				break;
			case WA_UPPAGE:
				amount = -page_h;
				break;
			case WA_DNPAGE:
				amount = page_h;
				break;
			}
			yy = top_row * row_height + amount;
			vscroll_to(yy);

			amount = 0;
			page_w = ((gr.g_w - MAIN_X_MARGIN) / col_width) * col_width;
			switch (message[4])
			{
			case WA_LFLINE:
				amount = -col_width;
				break;
			case WA_RTLINE:
				amount = col_width;
				break;
			case WA_LFPAGE:
				amount = -page_w;
				break;
			case WA_RTPAGE:
				amount = page_w;
				break;
			}
			xx = left_col * col_width + amount;
			hscroll_to(xx);
		}
		break;
		
	case WM_VSLID:
		if (message[3] == mainwin)
		{
			GRECT gr;
			long hh;
			long page_h;
			long yy;
			
			wind_get_grect(mainwin, WF_WORKXYWH, &gr);
			hh = row_height * char_rows;
			page_h = ((gr.g_h - MAIN_Y_MARGIN) / row_height) * row_height;
			yy = ((long)message[4] * (hh - page_h)) / 1000;
			vscroll_to(yy);
		}
		break;
		
	case WM_HSLID:
		if (message[3] == mainwin)
		{
			GRECT gr;
			long ww;
			long page_w;
			long xx;
			
			wind_get_grect(mainwin, WF_WORKXYWH, &gr);
			ww = col_width * CHAR_COLUMNS;
			page_w = ((gr.g_w - MAIN_X_MARGIN) / col_width) * col_width;
			xx = ((long)message[4] * (ww - page_w)) / 1000;
			hscroll_to(xx);
		}
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
		case FINFO:
			font_info();
			break;
		case POINTS_6:
			set_pointsize(60);
			break;
		case POINTS_8:
			set_pointsize(80);
			break;
		case POINTS_9:
			set_pointsize(90);
			break;
		case POINTS_10:
			set_pointsize(100);
			break;
		case POINTS_11:
			set_pointsize(110);
			break;
		case POINTS_12:
			set_pointsize(120);
			break;
		case POINTS_14:
			set_pointsize(140);
			break;
		case POINTS_16:
			set_pointsize(160);
			break;
		case POINTS_18:
			set_pointsize(180);
			break;
		case POINTS_24:
			set_pointsize(240);
			break;
		case POINTS_36:
			set_pointsize(360);
			break;
		case POINTS_48:
			set_pointsize(480);
			break;
		case POINTS_64:
			set_pointsize(640);
			break;
		case POINTS_72:
			set_pointsize(720);
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

/* ------------------------------------------------------------------------- */

static void mainloop(void)
{
	_WORD event;
	_WORD message[8];
	_WORD k, kstate, dummy, mox, moy;

	if (!is_font_loaded())
		msg_mn_select(TFILE, FOPEN);
	
	while (!quit_app)
	{
		menu_ienable(menu, FINFO, is_font_loaded());
		menu_ienable(menu, POINTS_6, is_font_loaded());
		menu_ienable(menu, POINTS_8, is_font_loaded());
		menu_ienable(menu, POINTS_9, is_font_loaded());
		menu_ienable(menu, POINTS_10, is_font_loaded());
		menu_ienable(menu, POINTS_11, is_font_loaded());
		menu_ienable(menu, POINTS_12, is_font_loaded());
		menu_ienable(menu, POINTS_14, is_font_loaded());
		menu_ienable(menu, POINTS_16, is_font_loaded());
		menu_ienable(menu, POINTS_18, is_font_loaded());
		menu_ienable(menu, POINTS_24, is_font_loaded());
		menu_ienable(menu, POINTS_36, is_font_loaded());
		menu_ienable(menu, POINTS_48, is_font_loaded());
		menu_ienable(menu, POINTS_64, is_font_loaded());
		menu_ienable(menu, POINTS_72, is_font_loaded());

		event = evnt_multi(MU_KEYBD | MU_MESAG | MU_BUTTON,
			2, 1, 1,
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
			case 0x09:
				font_info();
				break;
			case 0x11:
				quit_app = TRUE;
				break;
			default:
				{
					long page_h;
					long page_w;
					GRECT gr;
					
					wind_get_grect(mainwin, WF_WORKXYWH, &gr);
					page_w = ((gr.g_w - MAIN_X_MARGIN) / col_width) * col_width;
					page_h = ((gr.g_h - MAIN_Y_MARGIN) / row_height) * row_height;
					switch ((k >> 8) & 0xff)
					{
					case 0x48: /* cursor up */
						vscroll_to(top_row * row_height - ((kstate & K_SHIFT) ? page_h : row_height));
						break;
					case 0x50: /* cursor down */
						vscroll_to(top_row * row_height + ((kstate & K_SHIFT) ? page_h : row_height));
						break;
					case 0x49: /* page up */
						vscroll_to(top_row * row_height - page_h);
						break;
					case 0x51: /* page down */
						vscroll_to(top_row * row_height + page_h);
						break;
					case 0x4b: /* cursor left */
						if (kstate & (K_SHIFT|K_CTRL))
						{
							hscroll_to(0);
							vscroll_to(0);
						} else
						{
							hscroll_to(left_col * col_width - ((kstate & K_SHIFT) ? page_w : col_width));
						}
						break;
					case 0x4d: /* cursor right */
						if (kstate & (K_SHIFT|K_CTRL))
						{
							hscroll_to(0);
							vscroll_to(char_rows * row_height);
						} else
						{
							hscroll_to(left_col * col_width + ((kstate & K_SHIFT) ? page_w : col_width));
						}
						break;
					case 0x47: /* Home */
					case 0x77: /* Ctrl-Home */
						hscroll_to(0);
						vscroll_to(0);
						break;
					case 0x4f: /* End */
					case 0x75: /* Ctrl-End */
						hscroll_to(0);
						vscroll_to(char_rows * row_height);
						break;
					}
				}
				break;
			}
		}
		
		if (event & MU_MESAG)
		{
			handle_message(message, mox, moy);
		}
		
		if (event & MU_BUTTON)
		{
			_WORD win = wind_find(mox, moy);
			if (win == mainwin)
			{
				mainwin_click(mox, moy);
			} else if (win == previewwin)
			{
				preview_click(mox, moy);
			}
		}
	}
}

/* ------------------------------------------------------------------------- */

static void load_deffont(void)
{
	const char *filename = "..\\speedo\\btfonts\\bx000003.spd";
	struct stat st;
	
	if (stat(filename, &st) == 0)
		font_load_speedo_font(filename);
}

/* ------------------------------------------------------------------------- */

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
	
	spdview_rsc_load();
	
	if (!quit_app)
	{
		if (!open_screen() ||
			!create_preview_window() ||
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
		set_pointsize(point_size);
		if (argc > 1)
			font_load_speedo_font(argv[1]);
		else
			load_deffont();
	}
	
	graf_mouse(ARROW, NULL);
	
	mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
