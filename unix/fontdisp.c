/* gcc -O2 -Wall fontdisp.c -lX11 */
/* i686-pc-mingw32-gcc -mwin32 -O2 -Wall fontdisp.c -lgdi32 -lcomdlg32 */

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define INLINE __inline

#include "s_endian.h"
#include "fonthdr.h"

#undef access


#define g_malloc(s) malloc(s)
#define g_malloc0(s) calloc(1, s)
#define g_free(p) free(p)

typedef struct {
	short lbearing;			/* origin to left edge of raster */
	short rbearing;			/* origin to right edge of raster */
	short width;			/* advance to next char's origin */
	short ascent;			/* baseline to top edge of raster */
	short descent;			/* baseline to bottom edge of raster */
} vdi_charinfo;


#ifdef __WIN32__
#define VDI_DRIVER_WIN32
static HDC *scaled_pixmaps;
#define GetInstance() ((HINSTANCE)GetModuleHandle(NULL))
#else
#define VDI_DRIVER_X
#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
static Display *x_display;
static int screen;
static Visual *visual;
static XImage **char_images;
static Pixmap *scaled_pixmaps;
#endif
static UB *bitmap;
static int bitmap_line_width;
static UB *off_table;


#define VDI_FONTNAMESIZE 32

typedef struct _font_desc {
	char name[VDI_FONTNAMESIZE];
	int charset;
	int cellwidth;
	int cellheight;
	int width;
	int height;
	int top;
	int ascent;
	int half;
	int descent;
	int bottom;
	int left_offset;
	int right_offset;
	int pointsize;
	int first_char;
	int last_char;
	int default_char;
	int thicken;
	int underline_size;
	int lighten;
	int skew;
	
	gboolean monospaced;
	gboolean all_chars_exist;
	gboolean scaled;
	int font_id;
	int font_index;
	vdi_charinfo *per_char;
} FONT_DESC;

#undef SYSFONTS
#define SYSFONTS 4

static FONT_DESC *sysfont;
static const char *const sysfontname[SYSFONTS] = {
	"system08.fnt",
	"system09.fnt",
	"system10.fnt",
	"system20.fnt"
};

static char const program_name[] = "fontdisp";
static int cw, ch;
static int scaled_margin = 2;
static int scale = 3;
static int scaled_w, scaled_h;


static void swap_gemfnt_header(UB *h, unsigned int l)
{
	UB *u;
	
	if (l < 84)
		return;
	SWAP_W(h + 0); /* font_id */
	SWAP_W(h + 2); /* point */
	/* skip name */
	for (u = h + 36; u < h + 68; u += 2) /* first_ade .. flags */
	{
		SWAP_W(u);
	}
	SWAP_L(h + 68); /* hor_table */
	SWAP_L(h + 72); /* off_table */
	SWAP_L(h + 76); /* dat_table */
	SWAP_W(h + 80); /* form_width */
	SWAP_W(h + 82); /* form_height */
}


/*
 * There are apparantly several fonts that have the Motorola flag set
 * but are stored in little-endian format.
 */
static gboolean check_gemfnt_header(UB *h, unsigned int l)
{
	UW firstc, lastc, points;
	UW cellwidth;
	UL dat_offset;
	UL off_table;
	
	if (l < 84)
		return FALSE;
	firstc = LM_UW(h + 36);
	lastc = LM_UW(h + 38);
	points = LM_UW(h + 2);
	if (lastc == 256)
	{
		lastc = 255;
	}
	if (firstc >= 0x2000 || lastc >= 0xff00 || firstc > lastc)
		return FALSE;
	if (points >= 0x300)
		return FALSE;
	if (LM_UL(h + 68) >= l)
		return FALSE;
	off_table = LM_UL(h + 72);
	if (off_table < 84 || (off_table + (lastc - firstc + 1) * 2) > l)
		return FALSE;
	dat_offset = LM_UL(h + 76);
	if (dat_offset < 84 || dat_offset >= l)
		return FALSE;
	cellwidth = LM_UW(h + 52);
	if (cellwidth == 0)
		return FALSE;
#if 0
	{
	UW form_width, form_height;
	form_width = LM_UW(h + 80);
	form_height = LM_UW(h + 82);
	if ((dat_offset + form_width * form_height) > l)
		return FALSE;
	}
#endif

	/* FIXME: maybe should warn about incorrect value if it was changed */
	SM_UW(h + 38, lastc);
	return TRUE;
}


#if BYTE_ORDER == BIG_ENDIAN
#  define HOST_BIG 1
#else
#  define HOST_BIG 0
#endif

#define FONT_BIG ((LM_UW(h + 66) & FONTF_BIGENDIAN) != 0)


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


#define MEMADDR(a) ((void *)((char *)h + (UL)(a)))



static void gen_glyph_line(UB *dat, int off, int w, UB *out)
{
	int b;
	UB inmask;
	int p;
	
	b = (off & 7);
	inmask = 0x80 >> b;
	p = off >> 3;
	while (w > 0)
	{
		if (dat[p] & inmask)
			out[p] |= inmask;
		inmask >>= 1;
		b++;
		if (b == 8)
		{
			p++;
			inmask = 0x80;
			b = 0;
		}
		w--;
	}
}


#ifdef VDI_DRIVER_X

static Pixmap scale_ximage(XImage *img, int scale)
{
	unsigned int x, y;
	unsigned width = img->width;
	unsigned int height = img->height;
	Pixmap p2;
	GC gc;
	int pixw = width ? width * scale : 1;
	int pixh = height ? height * scale : 1;
	
	p2 = XCreatePixmap(x_display, RootWindow(x_display, screen), pixw, pixh, img->depth);
	gc = XCreateGC(x_display, p2, 0, 0);

	XSetForeground(x_display, gc, WhitePixel(x_display, screen));
	XSetFillStyle(x_display, gc, FillSolid);
	XFillRectangle(x_display, p2, gc, 0, 0, pixw, pixh);
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
		{
			XSetForeground(x_display, gc, XGetPixel(img, x, y));
			XFillRectangle(x_display, p2, gc, x * scale, y * scale, scale, scale);
		}

	XFreeGC(x_display, gc);
	return p2;
}


static void create_scaled_images(FONT_DESC *sf)
{
	int i, numoffs;
	
	numoffs = sf->last_char - sf->first_char + 1;
	for (i = 0; i < numoffs; i++)
	{
		if (scaled_pixmaps[i])
		{
			XFreePixmap(x_display, scaled_pixmaps[i]);
			scaled_pixmaps[i] = 0;
		}
		if (char_images[i])
			scaled_pixmaps[i] = scale_ximage(char_images[i], scale);
	}
}
#endif


#ifdef VDI_DRIVER_WIN32

static void create_scaled_images(FONT_DESC *sf)
{
	int i, numoffs;
	HDC scrdc;
	HDC pixmap;
	HBITMAP cbm;
	HBRUSH blackbrush, whitebrush;
	int x, y;
	RECT rc;
	
	scrdc = GetDC(NULL);
	whitebrush = GetStockObject(WHITE_BRUSH);
	blackbrush = GetStockObject(BLACK_BRUSH);
	numoffs = sf->last_char - sf->first_char + 1;
	for (i = 0; i < numoffs; i++)
	{
		vdi_charinfo *charinfo = &sf->per_char[i];
		int width = charinfo->width;
		int height = sf->cellheight;
		int pixw = width ? width * scale : 1;
		int pixh = height ? height * scale : 1;

		if (scaled_pixmaps[i])
		{
			DeleteObject(scaled_pixmaps[i]);
			scaled_pixmaps[i] = 0;
		}
		if (width == F_NO_CHAR)
		{
			/* TODO: display a cross or similar */
			continue;
		}
		pixmap = CreateCompatibleDC(scrdc);

		cbm = CreateCompatibleBitmap(scrdc, pixw, pixh);
		SelectObject(pixmap, cbm);
		SetBkMode(pixmap, OPAQUE);

		rc.left = 0;
		rc.top = 0;
		rc.right = pixw;
		rc.bottom = pixh;
		FillRect(pixmap, &rc, whitebrush);
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				int o = LM_UW(off_table + 2 * i) + x;
				int b = o & 0x7;
				UB mask = 0x80 >> b;
				UB *dat = bitmap + y * bitmap_line_width + (o >> 3);
				
				if (*dat & mask)
				{
					rc.left = x * scale;
					rc.top = y * scale;
					rc.right = rc.left + scale;
					rc.bottom = rc.top + scale;
					FillRect(pixmap, &rc, blackbrush);
				}
			}
		}
		
		scaled_pixmaps[i] = pixmap;
	}
}
#endif



static UW get_width(int i, int numoffs)
{
	int off = LM_UW(off_table + 2 * i);
	unsigned short next;
	
	off = LM_UW(off_table + 2 * i);
	if (off != F_NO_CHAR)
	{
		for (next = i + 1; next <= numoffs; next++)
		{
			int nextoff = LM_UW(off_table + 2 * next);
			if (nextoff != F_NO_CHAR)
				return nextoff - off;
		}
		off = F_NO_CHAR;
	}
	return off;
}


static FONT_DESC *font_gen_gemfont(UB **m, const char *filename, unsigned int l)
{
	FONT_DESC *font;
	UW form_width, form_height;
	UW flags;
	UW firstc, lastc;
	int i, numoffs;
	UB *hor_table;
	UB *dat_table;
	UB *u;
	int bitmap_width, bitmap_height;
	UL dat_offset, off_offset, hor_offset;
	gboolean hor_table_valid;
	UB *h = *m;
	int decode_ok = TRUE;
	UW last_offset;
	
	if (!check_gemfnt_header(h, l))
	{
		swap_gemfnt_header(h, l);
		if (!check_gemfnt_header(h, l))
		{
			swap_gemfnt_header(h, l);
			errno = EINVAL;
			return NULL;
		} else
		{
			if (FONT_BIG)
			{
				fprintf(stderr, "%s: warning: %s: wrong endian flag in header\n", program_name, filename);
				/*
				 * font claims to be big-endian,
				 * but check succeded only after swapping:
				 * font apparently is little-endian, clear flag
				 */
				SM_UW(h + 66, LM_UW(h + 66) & ~FONTF_BIGENDIAN);
			}
		}
	} else
	{
		if (!FONT_BIG)
		{
			fprintf(stderr, "%s: warning: %s: wrong endian flag in header\n", program_name, filename);
			/*
			 * font claims to be little-endian,
			 * but check succeded without swapping:
			 * font apparently is big-endian, set flag
			 */
			SM_UW(h + 66, LM_UW(h + 66) | FONTF_BIGENDIAN);
		}
	}
	
	font = g_malloc0(sizeof(*font));
	
	hor_offset = LM_UL(h + 68);
	off_offset = LM_UL(h + 72);
	dat_offset = LM_UL(h + 76);
	
	/*
	 * fontheader at this point always big-endian
	 */
	
	chomp(font->name, (const char *)h + 4, VDI_FONTNAMESIZE);

	form_width = LM_UW(h + 80);
	form_height = LM_UW(h + 82);
	
	font->charset = 0xff;
	font->font_id = LM_W(h + 0);
	font->pointsize = LM_W(h + 2);
	font->scaled = FALSE;
	
	firstc = font->first_char = LM_UW(h + 36);
	lastc = font->last_char = LM_UW(h + 38);
	font->default_char = '?';
	font->height = font->top = LM_UW(h + 40);
	font->ascent = LM_UW(h + 42);
	font->half = LM_UW(h + 44);
	font->descent = LM_UW(h + 46);
	font->bottom = LM_UW(h + 48);
	font->width = LM_UW(h + 50);
	font->cellwidth = LM_UW(h + 52);
	font->cellheight = form_height;
	font->left_offset = LM_UW(h + 54);
	font->right_offset = LM_UW(h + 56);
	font->thicken = LM_UW(h + 58);
	font->underline_size = LM_UW(h + 60);
	font->lighten = LM_UW(h + 62);
	font->skew = LM_UW(h + 64);
	
	flags = LM_UW(h + 66);
	font->monospaced = (flags & FONTF_MONOSPACED) != 0;
	
	numoffs = lastc - firstc + 1;

	if (!(flags & FONTF_COMPRESSED))
	{
		if ((dat_offset + form_width * form_height) > l)
			fprintf(stderr, "%s: warning: %s: file may be truncated\n", program_name, filename);
	}
		
	if (!(flags & FONTF_COMPRESSED))
	{
		if (dat_offset > off_offset && (off_offset + (numoffs + 1) * 2) < dat_offset)
			fprintf(stderr, "%s: warning: %s: gap of %u bytes before data\n", program_name, filename, dat_offset - (off_offset + (numoffs + 1) * 2));
	}

	if (flags & FONTF_COMPRESSED)
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
		form_size = (size_t)form_width * form_height;
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
			decode_gemfnt(h + offset, compressed, form_width, form_height);
			free(compressed);
		}
	}
	
	hor_table = MEMADDR(hor_offset);
	off_table = MEMADDR(off_offset);
	dat_table = MEMADDR(dat_offset);

	if (!(flags & FONTF_BIGENDIAN))
	{
		for (u = off_table; u <= off_table + numoffs * 2; u += 2)
		{
			SWAP_W(u);
		}
	}

	last_offset = LM_UW(off_table + 2 * numoffs);
	if ((((last_offset + 15) >> 4) << 1) != form_width)
		fprintf(stderr, "%s: warning: %s: offset of last character %u does not match form_width %u\n", program_name, filename, last_offset, form_width);

	hor_table_valid = hor_offset != 0 && hor_offset < off_offset && (off_offset - hor_offset) >= (numoffs * 2);
	if ((flags & FONTF_HORTABLE) && hor_table_valid)
	{
		if (!(flags & FONTF_BIGENDIAN))
		{
			for (u = hor_table; u < hor_table + numoffs * 2; u += 2)
			{
				SWAP_W(u);
			}
		}
	} else
	{
		if (flags & FONTF_HORTABLE)
		{
			fprintf(stderr, "%s: warning: %s: flag for horizontal table set, but there is none\n", program_name, filename);
			flags &= ~FONTF_HORTABLE;
			SM_UW(h + 66, flags);
		} else if (hor_table_valid)
		{
			fprintf(stderr, "%s: warning: %s: offset table present but flag not set\n", program_name, filename);
		}
	}
	
	if (!decode_ok)
	{
		g_free(font);
		errno = EINVAL;
		return NULL;
	}
	
	/*
	 * fontdata at this point always big-endian
	 */
	font->per_char = g_malloc0(numoffs * sizeof(*(font->per_char)));
	scaled_pixmaps = g_malloc0(numoffs * sizeof(*scaled_pixmaps));
	
	bitmap_width = form_width * 8;
	bitmap_height = form_height;
	bitmap_line_width = ((bitmap_width + 31) / 32) * 4;
	bitmap = g_malloc0(bitmap_line_width * form_height);

	for (i = 0; i < numoffs; i++)
	{
		int j;
		int o;
		int w;
		vdi_charinfo *charinfo = &font->per_char[i];
		
		o = LM_UW(off_table + 2 * i);
		w = get_width(i, numoffs);
		if (w == F_NO_CHAR)
		{
			/* TODO: display a cross or similar */
			continue;
		}
		charinfo->lbearing = 0;
		charinfo->rbearing = w - 1;
		charinfo->width = w;
		charinfo->ascent = font->top;
		charinfo->descent = form_height - font->top;
		if (w == F_NO_CHAR || w == 0)
			continue;
		
		for (j = 0; j < form_height; j++)
		{
			gen_glyph_line(dat_table + j * form_width, o, w, bitmap + j * bitmap_line_width);
		}
	}

#ifdef VDI_DRIVER_X
	{
		XImage ximage;
		XImage *charimage;
		int x, y;
		char *char_data;
		int char_bytes_per_line;
		
		memset(&ximage, 0, sizeof(ximage));
		ximage.height = bitmap_height;
		ximage.width = bitmap_width;
		ximage.depth = 1;
		ximage.bits_per_pixel = 1;
		ximage.xoffset = 0;
		ximage.format = XYBitmap;
		ximage.data = (char *)bitmap;
		ximage.byte_order = MSBFirst;
		ximage.bitmap_unit = 8;
		ximage.bitmap_bit_order = MSBFirst;
		ximage.bitmap_pad = 8;
		ximage.bytes_per_line = bitmap_line_width;
		XInitImage(&ximage);
		
		char_images = g_malloc0(numoffs * sizeof(*char_images));
		
		for (i = 0; i < numoffs; i++)
		{
			vdi_charinfo *charinfo = &font->per_char[i];
			int o = LM_UW(off_table + 2 * i);
			
			if (charinfo->width == F_NO_CHAR || charinfo->width == 0)
				continue;
			char_bytes_per_line = charinfo->width * 4;

			char_data = g_malloc0(char_bytes_per_line * bitmap_height);
			
			charimage = XCreateImage(x_display, visual, DefaultDepth(x_display, screen), ZPixmap, 0, char_data, charinfo->width, font->cellheight, BitmapPad(x_display), char_bytes_per_line);
			
			for (y = 0; y < font->cellheight; y++)
			{
				for (x = 0; x < charinfo->width; x++)
				{
					unsigned long pixel = XGetPixel(&ximage, x + o, y);

					XPutPixel(charimage, x, y, pixel ? BlackPixel(x_display, screen) : WhitePixel(x_display, screen));
				}
			}
			
			char_images[i] = charimage;
		}
	}

	g_free(bitmap);
	
#endif

#ifdef VDI_DRIVER_WIN32
	{
		scaled_pixmaps = g_malloc0(numoffs * sizeof(*scaled_pixmaps));
		(void) bitmap_height;
	}
#endif

	return font;
}


static gboolean font_load_gemfont(const char *filename)
{
	FILE *in;
	unsigned int l;
	UB *m;
	UB *h;
	
	in = fopen(filename, "rb");
	if (in == NULL)
	{
		return FALSE;
	}
	fseek(in, 0, SEEK_END);
	l = ftell(in);
	fseek(in, 0, SEEK_SET);
	m = malloc(l);
	l = fread(m, 1, l, in);
	h = m;

	sysfont = font_gen_gemfont(&h, filename, l);
	fclose(in);

	if (sysfont != NULL)
	{
		FONT_DESC *sf = sysfont;
		
		printf("%s: %s:\n", filename, sf->name);
		printf(" height      : %d\n", sf->height);
		printf(" cellheight  : %d\n", sf->cellheight);
		printf("\n");
		printf(" top         : %d\n", sf->top);
		printf(" ascent      : %d\n", sf->ascent);
		printf(" half        : %d\n", sf->half);
		printf(" descent     : %d\n", sf->descent);
		printf(" bottom      : %d\n", sf->bottom);
		printf("\n");
		printf(" width       : %d\n", sf->width);
		printf(" cellwidth   : %d\n", sf->cellwidth);
		printf(" left_offset : %d\n", sf->left_offset);
		printf(" right_offset: %d\n", sf->right_offset);
		printf(" point       : %d\n", sf->pointsize);
		printf("\n");

		cw = sysfont->cellwidth;
		ch = sysfont->cellheight;
	
	}

	return sysfont != NULL;
}



#ifdef VDI_DRIVER_WIN32

static HWND GlMainHwnd;
static HINSTANCE hInstance;

static void draw_window(HDC hdc, RECT *rc)
{
	int x, y;
	FONT_DESC *sf = sysfont;
	HBRUSH redbrush;
	RECT line;
	
	SetBkMode(hdc, OPAQUE);
	SetTextAlign(hdc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
	SetBkColor(hdc, RGB(255, 255, 255));
	SetTextColor(hdc, RGB(0, 0, 0));

	FillRect(hdc, rc, GetStockObject(WHITE_BRUSH));

	redbrush = CreateSolidBrush(RGB(255, 0, 0));
	
	for (y = 0; y < 17; y++)
	{
		line.left = 0;
		line.top = y * (ch * scale + scaled_margin);
		line.right = line.left + scaled_w;
		line.bottom = line.top + scaled_margin;
		FillRect(hdc, &line, redbrush);
	}
	for (x = 0; x < 17; x++)
	{
		line.left = x * (cw * scale + scaled_margin);
		line.top = 0;
		line.right = line.left + scaled_margin;
		line.bottom = line.top + scaled_h;
		FillRect(hdc, &line, redbrush);
	}

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			int c = y * 16 + x;
			if (c < sf->first_char || c > sf->last_char)
				c = sf->default_char;
			{
				c -= sf->first_char;
				if (scaled_pixmaps[c])
					BitBlt(hdc, x * (cw * scale + scaled_margin) + scaled_margin, y * (ch * scale + scaled_margin) + scaled_margin, cw * scale, ch * scale, scaled_pixmaps[c], 0, 0, SRCCOPY);
			}
		}
	}
	
	DeleteObject(redbrush);
}


static void create_win(void)
{
	scaled_margin = scale == 1 ? 1 : 2;
	scaled_w = cw * 16 * scale + 17 * scaled_margin;
	scaled_h = ch * 16 * scale + 17 * scaled_margin;
	if (GlMainHwnd == 0)
	{
		GlMainHwnd = CreateWindowEx(
			WS_EX_OVERLAPPEDWINDOW,
			"bla",
			sysfont->name,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, scaled_w, scaled_h,
			NULL, /* parent */
			NULL, /* menu */
			hInstance,
			NULL);
	}
		
	{
		RECT r, pos;
		
		GetWindowRect(GlMainHwnd, &r);
		pos = r;
		r.right = r.left + scaled_w;
		r.bottom = r.top + scaled_h;
		AdjustWindowRectEx(&r, GetWindowLong(GlMainHwnd, GWL_STYLE), FALSE, GetWindowLong(GlMainHwnd, GWL_EXSTYLE));
		scaled_w = r.right - r.left;
		scaled_h = r.bottom - r.top;
		MoveWindow(GlMainHwnd, pos.left, pos.top, scaled_w, scaled_h, TRUE);
	}
	ShowWindow(GlMainHwnd, SW_SHOW);

	UpdateWindow(GlMainHwnd);
}



static LRESULT CALLBACK mainWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		GlMainHwnd = 0;
		return 0L;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			RECT rc;
	
			hdc = BeginPaint(hwnd, &ps);
			GetClientRect(hwnd, &rc);
			draw_window(hdc, &rc);
			EndPaint(hwnd, &ps);
		}
		return 0;
	
	case WM_KEYDOWN:
		switch (wparam)
		{
		case VK_UP:
			scale++;
			create_scaled_images(sysfont);
			create_win();
			break;
		case VK_DOWN:
			if (scale > 1)
			{
				scale--;
				create_scaled_images(sysfont);
				create_win();
			}
			break;
		}
		break;

	case WM_CHAR:
		switch (wparam)
		{
		case 'q':
		case 0x1b:
			DestroyWindow(hwnd);
			break;
		}
		break;
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}


int main(int argc, const char **argv)
{
	int font;
	const char *filename;
	MSG MainMsg;
	FONT_DESC *sf;
	int from_commandline;
	int fontok;
	
	hInstance = GetInstance();
	
	font = 2;
	if (argc > 1)
	{
		char *end = NULL;
		
		font = strtol(argv[1], &end, 0);
		if (end && *end == '\0' && font >= 0 && font < SYSFONTS)
			filename = sysfontname[font];
		else
			filename = argv[1];
		from_commandline = TRUE;
	} else
	{
		filename = NULL;
		from_commandline = FALSE;
	}
	
	fontok = FALSE;
	while (!fontok)
	{
		if (filename == NULL)
		{
			OPENFILENAME ofn;
			static char filenamebuf[256];
			
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hInstance = hInstance;
			ofn.lpstrFilter = "FontFiles\0*.fnt\0All Files\0*.*\0\0";
			ofn.lpstrFile = filenamebuf;
			ofn.nMaxFile = sizeof(filenamebuf);
			ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NONETWORKBUTTON;
			
			if (GetOpenFileName(&ofn) == 0)
				return 1;
			filename = filenamebuf;
		}
		if (!font_load_gemfont(filename))
		{
			const char *err = strerror(errno);
			if (from_commandline)
			{
				fprintf(stderr, "%s: cant load font %s: %s\n", program_name, filename, err);
			} else
			{
				char *buf = g_malloc(strlen(err) + strlen(filename) + 100);
				sprintf(buf, "cant load font %s: %s", filename, err);
				MessageBox(NULL, buf, program_name, MB_OK);
				g_free(buf);
			}
			filename = NULL;
			from_commandline = FALSE;
		} else
		{
			fontok = TRUE;
		}
	}
		
	{
		WNDCLASS wndclass;

		wndclass.style = CS_BYTEALIGNCLIENT | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wndclass.lpfnWndProc = mainWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = hInstance;
		wndclass.hIcon = LoadIcon(hInstance, "AAA_ICON_1" /* IDI_APPLICATION */ );
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "bla";

		RegisterClass(&wndclass);
	}

	sf = sysfont;
	create_scaled_images(sf);
	create_win();
	
	while (GetMessage(&MainMsg, NULL, 0, 0))
	{
		TranslateMessage(&MainMsg);
		DispatchMessage(&MainMsg);
	}

	if (GlMainHwnd)
		DestroyWindow(GlMainHwnd);
	
	UnregisterClass("bla", hInstance);
	
	return 0;
}

#else /* !__WIN32__ */

static Window win;
static Atom xa_wm_delete_window;
static Atom xa_wm_protocols;
static GC gc;

#define E_MASK \
	KeyPressMask | \
	KeyReleaseMask | \
	ButtonPressMask | \
	ButtonReleaseMask | \
	EnterWindowMask | \
	LeaveWindowMask | \
	PointerMotionMask | \
	ButtonMotionMask | \
	Button1MotionMask | \
	Button2MotionMask | \
	ExposureMask | \
	StructureNotifyMask | \
	ResizeRedirectMask | \
	SubstructureNotifyMask | \
	PropertyChangeMask


static void draw_window(void)
{
	int x, y;
	FONT_DESC *sf = sysfont;
	
	XSetFunction(x_display, gc, GXcopy);
	XSetFillStyle(x_display, gc, FillSolid);
	XSetForeground(x_display, gc, WhitePixel(x_display, screen));
	XFillRectangle(x_display, win, gc, 0, 0, scaled_w, scaled_h);
			
	XSetForeground(x_display, gc, 0xff0000);
	for (y = 0; y < 17; y++)
	{
		XFillRectangle(x_display, win, gc, 0, y * (ch * scale + scaled_margin), scaled_w, scaled_margin);
	}
	for (x = 0; x < 17; x++)
	{
		XFillRectangle(x_display, win, gc, x * (cw * scale + scaled_margin), 0, scaled_margin, scaled_h);
	}

	XSetForeground(x_display, gc, BlackPixel(x_display, screen));
	XSetBackground(x_display, gc, WhitePixel(x_display, screen));
	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			int c = y * 16 + x;
			if (c < sf->first_char || c > sf->last_char)
				c = sf->default_char;
			{
				c -= sf->first_char;
				if (scaled_pixmaps[c])
					XCopyArea(x_display, scaled_pixmaps[c], win, gc, 0, 0, cw * scale, ch * scale, x * (cw * scale + scaled_margin) + scaled_margin, y * (ch * scale + scaled_margin) + scaled_margin);
			}
		}
	}
}


static void create_win(void)
{
	int x, y;
	XSizeHints *hints;
	XSetWindowAttributes xa;
	unsigned long attrvaluemask;
	Atom protocols[1];

	hints = XAllocSizeHints();

	x = y = 0;
	if (win)
		XDestroyWindow(x_display, win);
	scaled_margin = scale == 1 ? 1 : 2;
	scaled_w = cw * 16 * scale + 17 * scaled_margin;
	scaled_h = ch * 16 * scale + 17 * scaled_margin;
	hints->x = x + 20;
	hints->y = y;
	
	hints->flags = USPosition;
	attrvaluemask = 0;
	win = XCreateWindow(x_display, RootWindow(x_display, screen), hints->x, hints->y, scaled_w, scaled_h, 0, DefaultDepth(x_display, screen), InputOutput, visual, attrvaluemask, &xa);
	XSetWMNormalHints(x_display, win, hints);
	
	XSelectInput(x_display, win, E_MASK);
	protocols[0] = xa_wm_delete_window;
	XSetWMProtocols(x_display, win, protocols, 1);
	gc = XCreateGC(x_display, win, 0, NULL);
	XSetGraphicsExposures(x_display, gc, False);
	XSetForeground(x_display, gc, BlackPixel(x_display, screen));
	XStoreName(x_display, win, sysfont->name);
	XMapWindow(x_display, win);
}


int main(int argc, const char **argv)
{
	int font;
	const char *filename;
	XEvent e;
	FONT_DESC *sf;
	
	font = 2;
	if (argc > 1)
	{
		char *end = NULL;
		
		font = strtol(argv[1], &end, 0);
		if (end && *end == '\0' && font >= 0 && font < SYSFONTS)
			filename = sysfontname[font];
		else
			filename = argv[1];
	} else
	{
		filename = sysfontname[font];
	}

	x_display = XOpenDisplay(NULL);
	if (x_display == 0)
	{
		fprintf(stderr, "%s: could not open display\n", program_name);
		return EXIT_FAILURE;
	}
	screen = XDefaultScreen(x_display);
	visual = DefaultVisual(x_display, screen);
	
	if (!font_load_gemfont(filename))
	{
		fprintf(stderr, "%s: cant load font %s: %s\n", program_name, filename, strerror(errno));
		return 1;
	}
	
	xa_wm_delete_window = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
	xa_wm_protocols = XInternAtom(x_display, "WM_PROTOCOLS", False);
	
	sf = sysfont;
	create_scaled_images(sf);
	
	create_win();
	
	while (win != None)
	{
		XNextEvent(x_display, &e);
		switch (e.xany.type)
		{
		case Expose:
			draw_window();
			break;
		
		case ClientMessage:
			if (e.xclient.window == win &&
				e.xclient.message_type == xa_wm_protocols &&
				e.xclient.format == 32 &&
				(Atom)e.xclient.data.l[0] == xa_wm_delete_window)
			{
				XDestroyWindow(x_display, win);
			}
			break;

		case DestroyNotify:
			if (e.xdestroywindow.window == win)
			{
				win = None;
			}
			break;

		case KeyPress:
			switch (XKeycodeToKeysym(x_display, e.xkey.keycode, 0))
			{
			case XK_Up:
				scale++;
				create_scaled_images(sf);
				create_win();
				break;
			case XK_Down:
				if (scale > 1)
				{
					scale--;
					create_scaled_images(sf);
					create_win();
				}
				break;
			case 'q':
			case XK_Escape:
				XDestroyWindow(x_display, win);
				break;
			}
			break;
		}
	}
	
	return 0;
}

#endif /* __WIN32__ */
