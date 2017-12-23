#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <mint/arch/nf_ops.h>
#include <stdint.h>
#include "spdview.h"
#define RSC_NAMED_FUNCTIONS 1
static _WORD gl_wchar, gl_hchar;
#define GetTextSize(w, h) *(w) = gl_wchar, *(h) = gl_hchar
#define hfix_objs(a, b, c)
#define hrelease_objs(a, b)
#include "spdview.rsh"
#include "speedo.h"

#undef SWAP_W
#undef SWAP_L
#define SWAP_W(s) s = cpu_swab16(s)
#define SWAP_L(s) s = cpu_swab32(s)

#define PANEL_Y_MARGIN 20
#define PANEL_X_MARGIN 20
#define PANEL_WKIND (NAME | CLOSER | MOVER)

#define MAIN_Y_MARGIN 10
#define MAIN_X_MARGIN 10
#define MAIN_WKIND (NAME | CLOSER | MOVER | SIZER | UPARROW | DNARROW | VSLIDE)

static _WORD app_id = -1;
static _WORD mainwin = -1;
static _WORD panelwin = -1;
static _WORD aeshandle;
static _WORD vdihandle = 0;
static MFDB screen_fdb;
static _WORD workout[57];
static _WORD xworkout[57];
static _BOOL quit_app;
static OBJECT *menu;
static const int scaled_margin = 1;

static _WORD font_cw;
static _WORD font_ch;

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
static _WORD row_height = 1;

static int point_size = 240;
static int x_res = 72;
static int y_res = 72;
static int quality = 0;
static specs_t specs;
static ufix16 char_index, char_id;
#define	MAX_BITS	1024
static _UWORD framebuffer[MAX_BITS >> 4][MAX_BITS];

#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	uint16_t width;
	uint16_t height;
	int16_t off_horz;
	int16_t off_vert;
	bbox_t bbox;
	MFDB bitmap;
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

/* ------------------------------------------------------------------------- */

#define ror(x) (((x) >> 1) | ((x) & 1 ? 0x80 : 0))

static void draw_char(uint16_t ch, _WORD x0, _WORD y0)
{
	_WORD pxy[8];
	_WORD colors[2];
	charinfo *c;
	
	if (font_buffer == NULL)
		return;
	if (ch >= num_ids || (c = &infos[ch])->bitmap.fd_addr == NULL)
		return;
	colors[0] = G_BLACK;
	colors[1] = G_WHITE;
	pxy[0] = 0;
	pxy[1] = 0;
	pxy[2] = c->width - 1;
	pxy[3] = c->height - 1;
	pxy[4] = x0;
	pxy[5] = y0;
	pxy[6] = x0 + pxy[2];
	pxy[7] = y0 + pxy[3];
	if (screen_fdb.fd_nplanes == 1)
		vro_cpyfm(vdihandle, S_ONLY, pxy, &c->bitmap, &screen_fdb);
	else
		vrt_cpyfm(vdihandle, MD_REPLACE, pxy, &c->bitmap, &screen_fdb, colors);
}

/* ------------------------------------------------------------------------- */

static void mainwin_draw(const GRECT *area)
{
	GRECT gr;
	GRECT work;
	_WORD pxy[8];
	_WORD x, y;
	_WORD scaled_w;
	long scaled_h;
	
	v_hide_c(vdihandle);
	wind_get_grect(mainwin, WF_WORKXYWH, &work);
	scaled_w = font_cw * CHAR_COLUMNS + (CHAR_COLUMNS + 1) * scaled_margin;
	scaled_h = row_height * char_rows + scaled_margin;
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
				pxy[2] = pxy[0] + scaled_w - 1;
				pxy[3] = pxy[1] + scaled_margin - 1;
				vr_recfl(vdihandle, pxy);
			}
			/*
			 * draw the vertical grid lines
			 */
			for (x = 0; x < (CHAR_COLUMNS + 1); x++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN + x * (font_cw + scaled_margin);
				pxy[1] = work.g_y + MAIN_Y_MARGIN;
				pxy[2] = pxy[0] + scaled_margin - 1;
				pxy[3] = pxy[1] + (_WORD)scaled_h - 1;
				vr_recfl(vdihandle, pxy);
			}
			
			for (y = 0; y < char_rows; y++)
			{
				long yy = MAIN_Y_MARGIN + (y - top_row) * row_height + scaled_margin;
				if ((yy + row_height) <= (MAIN_Y_MARGIN + scaled_margin))
					continue;
				if (yy >= work.g_h)
					break;
				for (x = 0; x < CHAR_COLUMNS; x++)
				{
					uint16_t c = y * CHAR_COLUMNS + x;
					draw_char(c, work.g_x + MAIN_X_MARGIN + x * (font_cw + scaled_margin) + scaled_margin, work.g_y + (_WORD)yy);
				}
			}
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}

/* ------------------------------------------------------------------------- */

static void panelwin_draw(const GRECT *area)
{
	GRECT gr;
	OBJECT *panel = rs_tree(PANEL);
	
	wind_get_grect(panelwin, WF_WORKXYWH, &gr);
	panel[ROOT].ob_x = gr.g_x;
	panel[ROOT].ob_y = gr.g_y;
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

/* ------------------------------------------------------------------------- */

static _BOOL open_screen(void)
{
	_WORD work_in[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };
	_WORD dummy;
	
	vdihandle = aeshandle;
	(void) v_opnvwk(work_in, &vdihandle, workout);	/* VDI workstation needed */
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
		do_alert1(AL_NOWINDOW);
		return FALSE;
	}

	wind_set_str(panelwin, WF_NAME, "System Font");
	wind_open_grect(panelwin, &gr);
	
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

static _BOOL create_window(void)
{
	GRECT gr, desk, panel;
	_WORD wkind;
	_WORD width;
	long height;
	
	wind_get_grect(DESK, WF_WORKXYWH, &desk);

	width = MAIN_X_MARGIN * 2 + font_cw * CHAR_COLUMNS + (CHAR_COLUMNS + 1) * scaled_margin;
	height = MAIN_Y_MARGIN * 2 + row_height * char_rows + scaled_margin;
	if (height > desk.g_h)
		height = desk.g_h;
	
	/* create a window */
	gr.g_x = 100;
	gr.g_y = 100;
	gr.g_w = width;
	gr.g_h = (_WORD)height;
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	wind_get_grect(panelwin, WF_CURRXYWH, &panel);
	gr.g_x = panel.g_x + panel.g_w + 20;
	gr.g_y = panel.g_y;
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
	width = MAIN_X_MARGIN * 2 + font_cw * CHAR_COLUMNS + (CHAR_COLUMNS + 1) * scaled_margin;
	height = MAIN_Y_MARGIN * 2 + row_height * char_rows + scaled_margin;
	if (height > desk.g_h)
		height = desk.g_h;
	
	wind_get_grect(mainwin, WF_CURRXYWH, &gr);
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	gr.g_w = width;
	gr.g_h = (_WORD)height;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	wind_set_grect(mainwin, WF_CURRXYWH, &gr);
	calc_vslider();
	
	return TRUE;
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
	fix15 off_horz;
	fix15 off_vert;
	fix31 width, pix_width;
	bbox_t bb;

	bit_width = xsize;
	bit_height = ysize;

	off_horz = (fix15) ((xorg + 32768L) >> 16);
	off_vert = (fix15) ((yorg + 32768L) >> 16);

	if (bit_width > MAX_BITS)
	{
		nf_debugprintf("char 0x%x (0x%x) wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	width = sp_get_char_width(char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 7200L) / (point_size * y_res);

	sp_get_char_bbox(char_index, &bb);
	infos[char_id].bbox = bb;

#if DEBUG
	if (((bb.xmax - bb.xmin) >> 16) != bit_width)
		nf_debugprintf("char 0x%x (0x%x): bbox & width mismatch (%ld vs %d)\n",
				char_index, char_id, (unsigned long)(bb.xmax - bb.xmin) >> 16, bit_width);
	if (((bb.ymax - bb.ymin) >> 16) != bit_height)
		nf_debugprintf("char 0x%x (0x%x): bbox & height mismatch (%ld vs %d)\n",
				char_index, char_id, (unsigned long)(bb.ymax - bb.ymin) >> 16, bit_height);
	if ((bb.xmin >> 16) != off_horz)
		nf_debugprintf("char 0x%x (0x%x): x min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.xmin >> 16, off_horz);
	if ((bb.ymin >> 16) != off_vert)
		nf_debugprintf("char 0x%x (0x%x): y min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.ymin >> 16, off_vert);
#endif

	bit_width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
	bit_height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
	off_horz = bb.xmin >> 16;
	off_vert = bb.ymin >> 16;

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
	
	infos[char_id].width = bit_width;
	infos[char_id].height = bit_height;
	infos[char_id].off_horz = off_horz;
	infos[char_id].off_vert = off_vert;
	
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
boolean sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		nf_debugprintf("can't seek to char\n");
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
	char_data->org = (ufix8 *) c_buffer + cb_offset;
	char_data->no_bytes = num;

	return TRUE;
}
#endif

/* ------------------------------------------------------------------------- */

void sp_close_bitmap(void)
{
	charinfo *c = &infos[char_id];
	fix15 y;
	_UWORD *bitmap;
	
	c->bitmap.fd_wdwidth = (bit_width + 15) >> 4;
	c->bitmap.fd_w = bit_width;
	c->bitmap.fd_h = bit_height;
	c->bitmap.fd_nplanes = 1;
	c->bitmap.fd_stand = 1;
	c->bitmap.fd_r1 = c->bitmap.fd_r2 = c->bitmap.fd_r3 = 0;
	bitmap = g_new(_UWORD, c->bitmap.fd_wdwidth * c->bitmap.fd_h);
	c->bitmap.fd_addr = bitmap;
	if (bitmap != NULL)
	{
		for (y = 0; y < c->bitmap.fd_h; y++)
		{
			memcpy(bitmap, &framebuffer[y][0], c->bitmap.fd_wdwidth << 1);
			bitmap += c->bitmap.fd_wdwidth;
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

void sp_start_new_char(void)
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
	specs.pfont = &font;
	/* XXX beware of overflow */
	specs.xxmult = (long)point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = (long)point_size * y_res / 720 * (1L << 16);
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
	
	if (!sp_set_specs(&specs))
	{
		decode_ok = FALSE;
	} else
	{
		num_ids = 0;
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != 0 && char_id != 0xffff && char_id >= num_ids)
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
			infos[i].char_id = 0xffff;
			infos[i].bitmap.fd_addr = NULL;
		}

		/*
		 * determine the average width of all the glyphs in the font
		 */
		{
			bbox_t bb;
			unsigned long total_width;
			uint16_t num_glyphs;
			fix15 width, height;
			fix15 max_width, max_height;
			bbox_t max_bb;
			
			total_width = 0;
			num_glyphs = 0;
			max_width = 0;
			max_height = 0;
			max_bb.xmin = (32000L << 16);
			max_bb.xmax = -(32000L << 16);
			max_bb.ymin = (32000L << 16);
			max_bb.ymax = -(32000L << 16);
			for (i = 0; i < num_chars; i++)
			{
				char_index = i + first_char_index;
				char_id = sp_get_char_id(char_index);
				if (char_id != 0 && char_id != 0xffff)
				{
					sp_get_char_bbox(char_index, &bb);
					width = ((bb.xmax - bb.xmin) + 32768L) >> 16;
					height = ((bb.ymax - bb.ymin) + 32768L) >> 16;
					total_width += width;
					max_width = MAX(max_width, width);
					max_height = MAX(max_height, height);
					max_bb.xmin = MIN(max_bb.xmin, bb.xmin);
					max_bb.ymin = MIN(max_bb.ymin, bb.ymin);
					max_bb.xmax = MAX(max_bb.xmax, bb.xmax);
					max_bb.ymax = MAX(max_bb.ymax, bb.ymax);
					num_glyphs++;
				}
			}
			font_ch = max_height;
			if (num_glyphs == 0)
				font_cw = max_width;
			else
				font_cw = (_WORD)(total_width / num_glyphs);
			nf_debugprintf("max height %d max width %d average width %d\n", max_height, max_width, font_cw);
			nf_debugprintf("bbox: %7.2f %7.2f %7.2f %7.2f\n",
				(double)max_bb.xmin / 65536.0, (double)max_bb.ymin / 65536.0,
				(double)max_bb.xmax / 65536.0, (double)max_bb.ymax / 65536.0);
			nf_debugprintf("bbox (header): %d %d %d %d\n",
				read_2b(font_buffer + FH_FXMIN), read_2b(font_buffer + FH_FYMIN),
				read_2b(font_buffer + FH_FXMAX), read_2b(font_buffer + FH_FYMAX));
			font_cw = max_width;
			row_height = font_ch + scaled_margin;
		}
		
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != 0 && char_id != 0xffff)
			{
				if (infos[char_id].char_id != 0xffff)
				{
					nf_debugprintf("char 0x%x (0x%x) already defined\n", char_index, char_id);
				} else
				{
					infos[char_id].char_index = char_index;
					infos[char_id].char_id = char_id;
					if (!sp_make_char(char_index))
					{
						nf_debugprintf("can't make char %d (%x)\n", char_index, char_id);
					}
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
	ufix8 tmp[16];
	ufix32 minbufsize;
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		char buf[256];
		
		sprintf(buf, rs_str(AL_FOPEN), filename);
		do_alert(buf);
		return FALSE;
	}
	if (fread(tmp, sizeof(ufix8), 16, fp) != 16)
	{
		fclose(fp);
		do_alert1(AL_READERROR);
		return FALSE;
	}
	free(font_buffer);
	free(c_buffer);
	c_buffer = NULL;
	minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
	font_buffer = (ufix8 *) malloc(minbufsize);
	if (font_buffer == NULL)
	{
		fclose(fp);
		do_alert1(AL_NOMEM);
		return FALSE;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(font_buffer, sizeof(ufix8), minbufsize, fp) != minbufsize)
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
		font.org = font_buffer;
		top_row = 0;
		resize_window();
		wind_set_str(mainwin, WF_NAME, fontname);
		redraw_win(mainwin);
	} else
	{
		free(font_buffer);
		font_buffer = NULL;
		free(c_buffer);
		c_buffer = NULL;
		g_free(infos);
		infos = NULL;
		num_ids = 0;
		char_rows = 0;
	}

	return ret;
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
	static char mask[128] = "*.FNT";
	
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

/* ------------------------------------------------------------------------- */

static void font_info(void)
{
}

/* ------------------------------------------------------------------------- */

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
	tree[ret].ob_state &= ~OS_SELECTED;
	form_dial_grect(FMD_FINISH, &gr, &gr);
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

static void mainloop(void)
{
	_WORD event;
	_WORD message[8];
	_WORD k, kstate, dummy, mox, moy;

	if (font_buffer == NULL)
		msg_mn_select(TFILE, FOPEN);
	
	while (!quit_app)
	{
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
				switch ((k >> 8) & 0xff)
				{
				case 0x48:
					vscroll_to(top_row * row_height - row_height);
					break;
				case 0x50:
					vscroll_to(top_row * row_height + row_height);
					break;
				}
				break;
			}
		}
		
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
					wind_set_grect(message[3], WF_CURRXYWH, (GRECT *)&message[4]);
					calc_vslider();
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
					wind_set_int(message[3], WF_TOP, 0);
				break;

			case WM_REDRAW:
				if (message[3] == mainwin)
					mainwin_draw((const GRECT *)&message[4]);
				else if (message[3] == panelwin)
					panelwin_draw((const GRECT *)&message[4]);
				break;
			
			case WM_ARROWED:
				if (message[3] == mainwin)
				{
					long amount = 0;
					GRECT gr;
					long yy;
					long page_h;
					
					wind_get_grect(mainwin, WF_WORKXYWH, &gr);
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
				case FQUIT:
					quit_app = TRUE;
					break;
				}
				menu_tnormal(menu, message[3], TRUE);
				break;
			}
			wind_update(END_UPDATE);
		}
	}
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
	
	font_cw = gl_wchar;
	font_ch = gl_hchar;
	
	spdview_rsc_load();
	
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
			font_load_speedo_font(argv[1]);
		else
			font_load_speedo_font("..\\speedo\\btfonts\\bx000003.spd");
	}
	
	graf_mouse(ARROW, NULL);
	
	mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
