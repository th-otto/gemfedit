#include <gem.h>
#include <osbind.h>
#include <mintbind.h>
#include <time.h>
#include <mint/arch/nf_ops.h>
#include <stdint.h>
#include "fontdisp.h"
#define RSC_NAMED_FUNCTIONS 1
static _WORD gl_wchar, gl_hchar;
#define GetTextSize(w, h) *(w) = gl_wchar, *(h) = gl_hchar
#define hfix_objs(a, b, c)
#define hrelease_objs(a, b)
#include "fontdisp.rsh"
#include "s_endian.h"
#include "fonthdr.h"

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
static FONT_HDR fonthdr;
static unsigned char *fontmem;
static unsigned char *dat_table;
static uint16_t *off_table;
static unsigned char *hor_table;
static char fontname[VDI_FONTNAMESIZE + 1];
static unsigned short numoffs;

#define FONT_BIG ((fonthdr.flags & FONTF_BIGENDIAN) != 0)

static char const program_name[] = "fontdisp";


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
	_WORD x, y;
	_WORD width, height;
	unsigned short o;
	int b;
	unsigned char mask;
	unsigned char *dat, *p;
	
	if (dat_table == NULL)
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
				
				pxy[0] = x0 + x * scalex;
				pxy[1] = y0 + y * scaley;
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
					if (c >= fonthdr.first_ade && c <= fonthdr.last_ade)
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
	if (l < 84)
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
	UW cellwidth;
	UL dat_offset;
	UL off_table;
	
	if (l < 84)
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
	if (off_table < 84 || (off_table + (lastc - firstc + 1) * 2) > l)
		return FALSE;
	dat_offset = h->dat_table;
	if (dat_offset < 84 || dat_offset >= l)
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





static _BOOL font_gen_gemfont(unsigned char **m, const char *filename, unsigned long l)
{
	FONT_HDR *hdr = &fonthdr;
	uint16_t *u;
	_BOOL hor_table_valid;
	uint32_t dat_offset, off_offset, hor_offset;
	int decode_ok = TRUE;
	uint16_t last_offset;
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
				nf_debugprintf("%s: warning: %s: wrong endian flag in header\n", program_name, filename);
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
					 * font apparently is big-endian, set flag
					 */
					hdr->flags |= FONTF_BIGENDIAN;
				}
			}
		}
	} else
	{
		if (!FONT_BIG)
		{
			nf_debugprintf("%s: warning: %s: wrong endian flag in header\n", program_name, filename);
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
	
	numoffs = hdr->last_ade - hdr->first_ade + 1;

	hor_offset = hdr->hor_table;
	off_offset = hdr->off_table;
	dat_offset = hdr->dat_table;

	if (!(hdr->flags & FONTF_COMPRESSED))
	{
		if ((dat_offset + (size_t)hdr->form_width * hdr->form_height) > l)
			nf_debugprintf("%s: warning: %s: file may be truncated\n", program_name, filename);
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
			fprintf(stderr, "%s: warning: %s: compressed size %lu > uncompressed size %lu\n", program_name, filename, (unsigned long)compressed_size, (unsigned long)font_file_data_size);
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
	
	off_table = (uint16_t *)(h + off_offset);
	dat_table = h + dat_offset;

	if (FONT_BIG != HOST_BIG)
	{
		for (u = off_table; u <= off_table + numoffs; u++)
		{
			SWAP_W(*u);
		}
	}
	
	last_offset = off_table[numoffs];
	if ((((last_offset + 15) >> 4) << 1) != hdr->form_width)
		nf_debugprintf("%s: warning: %s: offset of last character %u does not match form_width %u\n", program_name, filename, last_offset, hdr->form_width);

	hor_table_valid = hor_offset != 0 && hor_offset < off_offset && (off_offset - hor_offset) >= (numoffs * 2);
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

	font_cw = hdr->max_cell_width;
	font_ch = hdr->form_height;
	
	return decode_ok;
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

	if (ret)
	{
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

		free(fontmem);
		fontmem = h;
		resize_window();
		wind_set_str(mainwin, WF_NAME, fontname);
		redraw_win(mainwin);
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
	font_load_gemfont(path);
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
	tree[ret].ob_state &= ~OS_SELECTED;
	
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

	if (dat_table == NULL)
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
	
	fontdisp_rsc_load();
	
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
	
	mainloop();
	
	destroy_win();
	cleanup();
	
	return retval;
}
