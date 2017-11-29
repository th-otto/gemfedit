/* i686-pc-mingw32-gcc -mwin32 -O2 -Wall fontdisp.c -lgdi32 -lcomdlg32 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include "../include/fonthdr.h"


typedef unsigned char UB;
typedef   signed char B;
typedef unsigned short UW;
typedef   signed short W;
typedef unsigned int UL;
typedef   signed int L;

typedef int gboolean;
#define FALSE 0
#define TRUE  1


#define INLINE __inline

static INLINE B *TO_B(void *s) { return (B *)s; }
static INLINE UB *TO_UB(void *s) { return (UB *)s; }
static INLINE W *TO_W(void *s) { return (W *)s; }
static INLINE UW *TO_UW(void *s) { return (UW *)s; }
static INLINE L *TO_L(void *s) { return (L *)s; }
static INLINE UL *TO_UL(void *s) { return (UL *)s; }

/* Load/Store primitives (without address checking) */
#define LOAD_B(_s) (*(TO_B(_s)))
#define LOAD_UB(_s) (*(TO_UB(_s)))
#define LOAD_W(_s) (*(TO_W(_s)))
#define LOAD_UW(_s) (*(TO_UW(_s)))
#define LOAD_L(_s) (*(TO_L(_s)))
#define LOAD_UL(_s) (*(TO_UL(_s)))

#define STORE_B(_d,_v) *(TO_B(_d)) = _v
#define STORE_UB(_d,_v)	*(TO_UB(_d)) = _v
#define STORE_W(_d,_v) *(TO_W(_d)) = _v
#define STORE_UW(_d,_v) *(TO_UW(_d)) = _v
#define STORE_L(_d,_v) *(TO_L(_d)) = _v
#define STORE_UL(_d,_v) *(TO_UL(_d)) = _v

static inline UW swap_w(UW x)
{
	return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

static inline UL swap_l(UL x)
{
	return ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));
}

#define SWAP_W(s) STORE_UW(s, ((UW)swap_w(LOAD_UW(s))))
#define SWAP_L(s) STORE_UL(s, ((UL)swap_l(LOAD_UL(s))))

#ifdef WORDS_BIGENDIAN

#define LM_W(s) LOAD_W(s)
#define LM_UW(s) LOAD_UW(s)
#define LM_L(s) LOAD_L(s)
#define LM_UL(s) LOAD_UL(s)
#define SM_W(d, v) STORE_W(d, v)
#define SM_UW(d, v) STORE_UW(d, v)
#define SM_L(d, v) STORE_L(d, v)
#define SM_UL(d, v) STORE_UL(d, v)

#else

#define LM_W(s) ((W)swap_w(LOAD_W(s)))
#define LM_UW(s) ((UW)swap_w(LOAD_UW(s)))
#define LM_L(s) ((L)swap_l(LOAD_L(s)))
#define LM_UL(s) ((UL)swap_l(LOAD_UL(s)))
#define SM_W(d, v) STORE_W(d, swap_w(v))
#define SM_UW(d, v) STORE_UW(d, swap_w(v))
#define SM_L(d, v) STORE_L(d, swap_l(v))
#define SM_UL(d, v) STORE_UL(d, swap_l(v))

#endif /* WORDS_BIGENDIAN */

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


static HDC *scaled_pixmaps;
#define GetInstance() ((HINSTANCE)GetModuleHandle(NULL))
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
static const int scaled_margin = 2;
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
	UW form_width, form_height;
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
	form_width = LM_UW(h + 80);
	form_height = LM_UW(h + 82);
	if ((dat_offset + form_width * form_height) > l)
		return FALSE;
	SM_UW(h + 38, lastc);
	return TRUE;
}


#ifdef WORDS_BIGENDIAN
#  define HOST_BIG 1
#else
#  define HOST_BIG 0
#endif

#define FONT_BIG ((LM_UW(h + 66) & 0x04) != 0)


static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen);
	dst[maxlen - 1] = '\0';
	len = strlen(dst);
	while (len > 0 && dst[len - 1] == ' ')
		dst[--len] = '\0';
}


#define VADDR(p) ((UL)((char *)(p) - (char *)h))
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
			DeleteObject(scaled_pixmaps[i]);
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


static FONT_DESC *font_gen_gemfont(UB *h, const char *filename, unsigned int l, gboolean fix_offsets)
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
				if (HOST_BIG)
				{
					/*
					 * host big-endian, font claims to be little-endian,
					 * but check succeded only after swapping:
					 * font apparently is big-endian, set flag
					 */
					SM_UW(h + 66, LM_UW(h + 66) | 0x0004);
				} else
				{
					/*
					 * host little-endian, font claims to be big-endian,
					 * but check succeded only after swapping:
					 * font apparently is little-endian, clear flag
					 */
					SM_UW(h + 66, LM_UW(h + 66) & ~0x0004);
				}
			}
		}
	} else
	{
		if (!FONT_BIG)
		{
			fprintf(stderr, "%s: warning: %s: wrong endian flag in header\n", program_name, filename);
			if (HOST_BIG)
			{
				/*
				 * host big-endian, font claims to be little-endian,
				 * but check succeded without swapping:
				 * font apparently is big-endian, set flag
				 */
				SM_UW(h + 66, LM_UW(h + 66) | 0x0004);
			} else
			{
				/*
				 * host little-endian, font claims to be big-endian,
				 * but check succeded without swapping:
				 * font apparently is little-endian, clear flag
				 */
				SM_UW(h + 66, LM_UW(h + 66) & ~0x0004);
			}
		}
	}
	
	font = g_malloc0(sizeof(*font));
	
	if (fix_offsets)
	{
		UL off;
		
		off = LM_UL(h + 68);
		if (off != 0)
			SM_UL(h + 68, VADDR(h) + off);
		off = LM_UL(h + 72);
		if (off != 0)
			SM_UL(h + 72, VADDR(h) + off);
		off = LM_UL(h + 76);
		if (off != 0)
			SM_UL(h + 76, VADDR(h) + off);
	}
		
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
	
	hor_table = MEMADDR(LM_UL(h + 68));
	off_table = MEMADDR(LM_UL(h + 72));
	dat_table = MEMADDR(LM_UL(h + 76));

	numoffs = lastc - firstc + 1;

	if (!(flags & FONTF_BIGENDIAN))
	{
		for (u = off_table; u <= off_table + numoffs * 2; u += 2)
		{
			SWAP_W(u);
		}
		if ((flags & FONTF_HORTABLE) && hor_table != h && hor_table != off_table && (off_table - hor_table) >= (numoffs * 2))
		{
			for (u = hor_table; u < hor_table + numoffs * 2; u += 2)
			{
				SWAP_W(u);
			}
		}
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
		int o = LM_UW(off_table + 2 * i);
		int w = LM_UW(off_table + 2 * i + 2) - o;
		vdi_charinfo *charinfo = &font->per_char[i];
		
		charinfo->lbearing = 0;
		charinfo->rbearing = w - 1;
		charinfo->width = w;
		charinfo->ascent = font->top;
		charinfo->descent = form_height - font->top;
		
		for (j = 0; j < form_height; j++)
		{
			gen_glyph_line(dat_table + j * form_width, o, w, bitmap + j * bitmap_line_width);
		}
	}

	{
		scaled_pixmaps = g_malloc0(numoffs * sizeof(*scaled_pixmaps));
		(void) bitmap_height;
	}

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

	sysfont = font_gen_gemfont(h, filename, l, TRUE);
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
				BitBlt(hdc, x * (cw * scale + scaled_margin) + scaled_margin, y * (ch * scale + scaled_margin) + scaled_margin, cw * scale, ch * scale, scaled_pixmaps[c], 0, 0, SRCCOPY);
			}
		}
	}
	
	DeleteObject(redbrush);
}


static void create_win(void)
{
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
