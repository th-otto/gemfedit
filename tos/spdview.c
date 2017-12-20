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
#define MAIN_WKIND (NAME | CLOSER | MOVER)

static _WORD app_id = -1;
static _WORD mainwin = -1;
static _WORD panelwin = -1;
static _WORD aeshandle;
static _WORD vdihandle = 0;
static _BOOL quit_app;
static OBJECT *menu;
static _WORD scalex = 3;
static _WORD scaley = 3;
static const int scaled_margin = 2;

static _WORD font_cw;
static _WORD font_ch;

static buff_t font;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static ufix16 mincharsize;
static char fontname[70 + 1];
static unsigned short first_char_index;
static unsigned short last_char_index;
static unsigned short num_chars;

static int point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 0;
static specs_t specs;
static ufix16 char_index, char_id;
#define	MAX_BITS	1024
static char line_of_bits[MAX_BITS][MAX_BITS + 1];
static buff_t char_data;

typedef struct {
	unsigned short char_index;
	unsigned short char_id;
} charinfo;


static OBJECT *rs_tree(_WORD num)
{
	return rs_trindex[num];
}

	
static char *rs_str(_WORD num)
{
	return rs_frstr[num];
}

	
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

static void cleanup(void)
{
	if (app_id >= 0)
	{
		menu_bar(menu, FALSE);
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


#define ror(x) (((x) >> 1) | ((x) & 1 ? 0x80 : 0))

static void draw_char(unsigned short c, _WORD x0, _WORD y0)
{
	if (font_buffer == NULL)
		return;
	if (c < first_char_index || c > last_char_index)
		return;
	(void) x0;
	(void) y0;
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
	scaled_w = font_cw * 16 * scalex + 17 * scaled_margin;
	scaled_h = font_ch * 16 * scaley + 17 * scaled_margin;
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
			for (y = 0; y < 17; y++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN;
				pxy[1] = work.g_y + MAIN_Y_MARGIN + y * (font_ch * scaley + scaled_margin);
				pxy[2] = pxy[0] + scaled_w - 1;
				pxy[3] = pxy[1] + scaled_margin - 1;
				vr_recfl(vdihandle, pxy);
			}
			for (x = 0; x < 17; x++)
			{
				pxy[0] = work.g_x + MAIN_X_MARGIN + x * (font_cw * scalex + scaled_margin);
				pxy[1] = work.g_y + MAIN_Y_MARGIN;
				pxy[2] = pxy[0] + scaled_margin - 1;
				pxy[3] = pxy[1] + scaled_h - 1;
				vr_recfl(vdihandle, pxy);
			}
			
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					unsigned short c = y * 16 + x;
					if (c >= first_char_index && c <= last_char_index)
						draw_char(c, work.g_x + MAIN_X_MARGIN + x * (font_cw * scalex + scaled_margin) + scaled_margin, work.g_y + MAIN_Y_MARGIN + y * (font_ch * scaley + scaled_margin) + scaled_margin);
				}
			}
		}
		
		wind_get_grect(mainwin, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdihandle, 1);
}


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
	
	width = MAIN_X_MARGIN * 2 + scalex * font_cw * 16 + 17 * scaled_margin;
	height = MAIN_Y_MARGIN * 2 + scaley * font_ch * 16 + 17 * scaled_margin;
	
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


static _BOOL resize_window(void)
{
	GRECT gr, desk;
	_WORD wkind;
	_WORD width, height;
	
	width = MAIN_X_MARGIN * 2 + scalex * font_cw * 16 + 17 * scaled_margin;
	height = MAIN_Y_MARGIN * 2 + scaley * font_ch * 16 + 17 * scaled_margin;
	
	wind_get_grect(mainwin, WF_CURRXYWH, &gr);
	wind_get_grect(DESK, WF_WORKXYWH, &desk);
	wkind = MAIN_WKIND;
	wind_calc_grect(WC_WORK, wkind, &gr, &gr);
	gr.g_w = width;
	gr.g_h = height;
	wind_calc_grect(WC_BORDER, wkind, &gr, &gr);
	rc_intersect(&desk, &gr);
	wind_set_grect(mainwin, WF_CURRXYWH, &gr);
	
	return TRUE;
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


static FILE *fp;
static fix15 bit_width, bit_height;

/*
 * Reads 1-byte field from font buffer 
 */
#define read_1b(pointer) (*(pointer))


static fix15 read_2b(ufix8 *ptr)
{
	fix15 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

static fix31 read_4b(ufix8 *ptr)
{
	fix31 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

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
	form_alert(1, msg);
}



void sp_open_bitmap(fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
	fix15 i, y;
	fix15 off_horz;
	fix15 off_vert;
	fix31 width, pix_width;
	bbox_t bb;

	UNUSED(x_set_width);
	UNUSED(y_set_width);
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
	bb.xmin >>= 16;
	bb.ymin >>= 16;
	bb.xmax >>= 16;
	bb.ymax >>= 16;

	if ((bb.xmax - bb.xmin) != bit_width)
		nf_debugprintf("bbox & width mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.xmax - bb.xmin), bit_width);
	if ((bb.ymax - bb.ymin) != bit_height)
		nf_debugprintf("bbox & height mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.ymax - bb.ymin), bit_height);
	if (bb.xmin != off_horz)
		nf_debugprintf("x min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.xmin, off_horz);
	if (bb.ymin != off_vert)
		nf_debugprintf("y min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.ymin, off_vert);

	bit_width = bb.xmax - bb.xmin;
	bit_height = bb.ymax - bb.ymin;
	off_horz = bb.xmin;
	off_vert = bb.ymin;

	/* XXX kludge to handle space */
	if (bb.xmin == 0 && bb.ymin == 0 && bb.xmax == 0 && bb.ymax == 0 && width)
	{
		bit_width = 1;
		bit_height = 1;
	}

	if (bit_width > MAX_BITS)
	{
		nf_debugprintf("width too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		nf_debugprintf("height too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_height = MAX_BITS;
	}
	
	for (y = 0; y < bit_height; y++)
	{
		for (i = 0; i < bit_width; i++)
		{
			line_of_bits[y][i] = ' ';
		}
		line_of_bits[y][bit_width] = '\0';
	}
}


static int trunc = 0;


void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 > MAX_BITS)
	{
		nf_debugprintf("run wider than max bits -- truncated\n");
		xbit1 = MAX_BITS;
	}
	if (xbit2 > MAX_BITS)
	{
		nf_debugprintf("run wider than max bits -- truncated\n");
		xbit2 = MAX_BITS;
	}

	if (y >= bit_height)
	{
		nf_debugprintf("y value is larger than height 0x%x (0x%x) -- truncated\n", char_index, char_id);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		line_of_bits[y][i] = '*';
	}
}


buff_t *sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset)
{
	if (fseek(fp, (long) file_offset, (int) 0))
	{
		nf_debugprintf("can't seek to char\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if ((num + cb_offset) > mincharsize)
	{
		nf_debugprintf("char buf overflow\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		nf_debugprintf("can't get char data\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	char_data.org = (ufix8 *) c_buffer + cb_offset;
	char_data.no_bytes = num;

	return &char_data;
}


static void dump_line(const char *line)
{
	int bit;
	unsigned byte;

	byte = 0;
	for (bit = 0; bit < bit_width; bit++)
	{
		if (line[bit] != ' ')
			byte |= (1 << (7 - (bit & 7)));
		if ((bit & 7) == 7)
		{
			printf("%02X", byte);
			byte = 0;
		}
	}
	if ((bit & 7) != 0)
		printf("%02X", byte);
	printf("\n");
}


void sp_close_bitmap(void)
{
	int y, i;

	trunc = 0;

	for (y = 0; y < bit_height; y++)
		dump_line(line_of_bits[y]);

	for (y = 0; y < bit_height; y++)
	{
		for (i = 0; i < bit_width; i++)
		{
			line_of_bits[y][i] = ' ';
		}
		line_of_bits[y][bit_width] = '\0';
	}
}


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

void sp_start_new_char(void)
{
}

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	UNUSED(x);
	UNUSED(y);
	UNUSED(outside);
}

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	UNUSED(x1);
	UNUSED(y1);
	UNUSED(x2);
	UNUSED(y2);
	UNUSED(x3);
	UNUSED(y3);
}

void sp_line_to(fix31 x1, fix31 y1)
{
	UNUSED(x1);
	UNUSED(y1);
}

void sp_close_contour(void)
{
}

void sp_close_outline(void)
{
}
#endif


static _BOOL font_gen_speedo_font(void)
{
	_BOOL decode_ok = TRUE;
	const ufix8 *key;
	unsigned short i;

	/* init */
	sp_reset();

	key = sp_get_key(&font);
	if (key == NULL)
	{
		sp_write_error("Non-standard encryption");
#if 0
		decode_ok = FALSE;
#endif
	} else
	{
		sp_set_key(key);
	}
	
	first_char_index = read_2b(font_buffer + FH_FCHRF);
	num_chars = read_2b(font_buffer + FH_NCHRL);
	last_char_index = first_char_index + num_chars - 1;
	
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
		specs.flags = 0;
		break;
	case 1:
		specs.flags = MODE_SCREEN;
		break;
	case 2:
		specs.flags = MODE_2D;
		break;
	}

	chomp(fontname, (char *) (font_buffer + FH_FNTNM), sizeof(fontname));
	
	if (!sp_set_specs(&specs))
	{
		decode_ok = FALSE;
	} else
	{
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id)
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
		form_alert(1, buf);
		return FALSE;
	}
	free(font_buffer);
	free(c_buffer);
	minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
	font_buffer = (ufix8 *) malloc(minbufsize);
	if (font_buffer == NULL)
	{
		fclose(fp);
		form_alert(1, rs_str(AL_NOMEM));
		return FALSE;
	}
	fseek(fp, (ufix32) 0, 0);
	if (fread(font_buffer, sizeof(ufix8), minbufsize, fp) != minbufsize)
	{
		fclose(fp);
		free(font_buffer);
		font_buffer = NULL;
		return FALSE;
	}

	mincharsize = read_2b(font_buffer + FH_CBFSZ);

	c_buffer = (ufix8 *) malloc(mincharsize);
	if (!c_buffer)
	{
		fclose(fp);
		free(font_buffer);
		font_buffer = NULL;
		form_alert(1, rs_str(AL_NOMEM));
		return 1;
	}

	font.org = font_buffer;
	font.no_bytes = minbufsize;

	ret = font_gen_speedo_font();
	fclose(fp);

	if (ret)
	{
		font.org = font_buffer;
		resize_window();
		wind_set_str(mainwin, WF_NAME, fontname);
		redraw_win(mainwin);
	} else
	{
		free(font_buffer);
		font_buffer = NULL;
		free(c_buffer);
		c_buffer = NULL;
	}

	return ret;
}


/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

static void font_info(void)
{
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
	tree[ret].ob_state &= ~OS_SELECTED;
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
					scalex++;
					scaley++;
					resize_window();
					redraw_win(mainwin);
					break;
				case 0x50:
					if (scaley > 1)
					{
						scalex--;
						scaley--;
						resize_window();
						redraw_win(mainwin);
					}
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
					wind_set_int(message[3], WF_TOP, 0);
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
			font_load_speedo_font("..\\speedo\\bx000003.spd");
	}
	
	graf_mouse(ARROW, NULL);
	
	mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
