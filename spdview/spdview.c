#include "linux/libcwrap.h"
#include "spddefs.h"
#include <curl/curl.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include "cgic.h"
#include "speedo.h"
#include "writepng.h"

char const gl_program_name[] = "spdview.cgi";
char const gl_program_version[] = "1.0";
static const char *cgi_scriptname = "spdview.cgi";
static char const spdview_css_name[] = "_spdview.css";
static char const spdview_js_name[] = "_spdview.js";
static char const charset[] = "UTF-8";

static char const cgi_cachedir[] = "cache";

static char const html_error_note_style[] = "spdview_error_note";
static char const html_node_style[] = "spdview_node";

struct curl_parms {
	const char *filename;
	FILE *fp;
};

#define ALLOWED_PROTOS ( \
	CURLPROTO_FTP | \
	CURLPROTO_FTPS | \
	CURLPROTO_HTTP | \
	CURLPROTO_HTTPS | \
	CURLPROTO_SCP | \
	CURLPROTO_SFTP | \
	CURLPROTO_TFTP)

static const char *html_closer = " />";
static char *output_dir;
static char *output_url;
static char *html_referer_url;
static FILE *errorfile;

#define _(x) x

static int force_crlf = FALSE;

#define	MAX_BITS	1024
#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

static unsigned char framebuffer[MAX_BITS][MAX_BITS];
static ufix16 char_index, char_id;
static buff_t font;
static buff_t char_data;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static ufix16 mincharsize;
static fix15 bit_width, bit_height;
static char fontname[70 + 1];
static char fontfilename[70 + 1];
static uint16_t first_char_index;
static uint16_t num_chars;

static long point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 1;

static specs_t specs;

#define HAVE_MKSTEMPS

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	char *local_filename;
	char *url;
	uint16_t width;
	uint16_t height;
	int16_t off_horz;
	int16_t off_vert;
} charinfo;
static charinfo *infos;

static unsigned char const vdi_maptab256[256] = {
    0, 255,   1,   2,   4,   6,   3,   5,   7,   8,   9,  10,  12,  14,  11,  13,
   16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
   32 , 33 , 34 , 35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
   48 , 49 , 50 , 51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
   64 , 65 , 66 , 67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
   80 , 81 , 82 , 83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
   96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
  144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
  160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
  176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
  192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
  208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
  224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
  240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,  15
};

static short const palette[256][3] = {
	{ 1000, 1000, 1000 }, /* 0: WHITE, hw 0 */
	{ 0, 0, 0 },          /* 1: BLACK, hw 15/255 */
	{ 1000, 0, 0 },       /* 2: RED, hw 1 */
	{ 0, 1000, 0 },       /* 3: GREEN, hw 2 */
	{ 0, 0, 1000 },       /* 4: BLUE, hw 4 */
	{ 0, 1000, 1000 },    /* 5: CYAN, hw 6 */
	{ 1000, 1000, 0 },    /* 6: YELLOW, hw 3 */
	{ 1000, 0, 1000 },    /* 7: MAGENTA, hw 5 */
	{ 800, 800, 800 },    /* 8: LWHITE, hw 7 */
	{ 533, 533, 533 },    /* 9: LBLACK, hw 8 */
	{ 533, 0, 0 },        /* 10: LRED, hw 9 */
	{ 0, 533, 0 },        /* 11: LGREEN, hw 10 */
	{ 0, 0, 533 },        /* 12: LBLUE, hw 12 */
	{ 0, 533, 533 },      /* 13: LCYAN, hw 14 */
	{ 533, 533, 0 },      /* 14: LYELLOW, hw 11 */
	{ 533, 0, 533 },      /* 15: LMAGENTA, hw 13 */
	{ 992, 992, 992 },
	{ 925, 925, 925 },
	{ 858, 858, 858 },
	{ 792, 792, 792 },
	{ 725, 725, 725 },
	{ 658, 658, 658 },
	{ 592, 592, 592 },
	{ 529, 529, 529 },
	{ 462, 462, 462 },
	{ 396, 396, 396 },
	{ 329, 329, 329 },
	{ 262, 262, 262 },
	{ 196, 196, 196 },
	{ 129, 129, 129 },
	{ 63, 63, 63 },
	{ 0, 0, 0 },
	{ 992, 0, 0 },
	{ 992, 0, 63 },
	{ 992, 0, 129 },
	{ 992, 0, 196 },
	{ 992, 0, 262 },
	{ 992, 0, 329 },
	{ 992, 0, 396 },
	{ 992, 0, 462 },
	{ 992, 0, 529 },
	{ 992, 0, 592 },
	{ 992, 0, 658 },
	{ 992, 0, 725 },
	{ 992, 0, 792 },
	{ 992, 0, 858 },
	{ 992, 0, 925 },
	{ 992, 0, 992 },
	{ 925, 0, 992 },
	{ 858, 0, 992 },
	{ 792, 0, 992 },
	{ 725, 0, 992 },
	{ 658, 0, 992 },
	{ 592, 0, 992 },
	{ 529, 0, 992 },
	{ 462, 0, 992 },
	{ 396, 0, 992 },
	{ 329, 0, 992 },
	{ 262, 0, 992 },
	{ 196, 0, 992 },
	{ 129, 0, 992 },
	{ 63, 0, 992 },
	{ 0, 0, 992 },
	{ 0, 63, 992 },
	{ 0, 129, 992 },
	{ 0, 196, 992 },
	{ 0, 262, 992 },
	{ 0, 329, 992 },
	{ 0, 396, 992 },
	{ 0, 462, 992 },
	{ 0, 529, 992 },
	{ 0, 592, 992 },
	{ 0, 658, 992 },
	{ 0, 725, 992 },
	{ 0, 792, 992 },
	{ 0, 858, 992 },
	{ 0, 925, 992 },
	{ 0, 992, 992 },
	{ 0, 992, 925 },
	{ 0, 992, 858 },
	{ 0, 992, 792 },
	{ 0, 992, 725 },
	{ 0, 992, 658 },
	{ 0, 992, 592 },
	{ 0, 992, 529 },
	{ 0, 992, 462 },
	{ 0, 992, 396 },
	{ 0, 992, 329 },
	{ 0, 992, 262 },
	{ 0, 992, 196 },
	{ 0, 992, 129 },
	{ 0, 992, 63 },
	{ 0, 992, 0 },
	{ 63, 992, 0 },
	{ 129, 992, 0 },
	{ 196, 992, 0 },
	{ 262, 992, 0 },
	{ 329, 992, 0 },
	{ 396, 992, 0 },
	{ 462, 992, 0 },
	{ 529, 992, 0 },
	{ 592, 992, 0 },
	{ 658, 992, 0 },
	{ 725, 992, 0 },
	{ 792, 992, 0 },
	{ 858, 992, 0 },
	{ 925, 992, 0 },
	{ 992, 992, 0 },
	{ 992, 925, 0 },
	{ 992, 858, 0 },
	{ 992, 792, 0 },
	{ 992, 725, 0 },
	{ 992, 658, 0 },
	{ 992, 592, 0 },
	{ 992, 529, 0 },
	{ 992, 462, 0 },
	{ 992, 396, 0 },
	{ 992, 329, 0 },
	{ 992, 262, 0 },
	{ 992, 196, 0 },
	{ 992, 129, 0 },
	{ 992, 63, 0 },
	{ 725, 0, 0 },
	{ 725, 0, 63 },
	{ 725, 0, 129 },
	{ 725, 0, 196 },
	{ 725, 0, 262 },
	{ 725, 0, 329 },
	{ 725, 0, 396 },
	{ 725, 0, 462 },
	{ 725, 0, 529 },
	{ 725, 0, 592 },
	{ 725, 0, 658 },
	{ 725, 0, 725 },
	{ 658, 0, 725 },
	{ 592, 0, 725 },
	{ 529, 0, 725 },
	{ 462, 0, 725 },
	{ 396, 0, 725 },
	{ 329, 0, 725 },
	{ 262, 0, 725 },
	{ 196, 0, 725 },
	{ 129, 0, 725 },
	{ 63, 0, 725 },
	{ 0, 0, 725 },
	{ 0, 63, 725 },
	{ 0, 129, 725 },
	{ 0, 196, 725 },
	{ 0, 262, 725 },
	{ 0, 329, 725 },
	{ 0, 396, 725 },
	{ 0, 462, 725 },
	{ 0, 529, 725 },
	{ 0, 592, 725 },
	{ 0, 658, 725 },
	{ 0, 725, 725 },
	{ 0, 725, 658 },
	{ 0, 725, 592 },
	{ 0, 725, 529 },
	{ 0, 725, 462 },
	{ 0, 725, 396 },
	{ 0, 725, 329 },
	{ 0, 725, 262 },
	{ 0, 725, 196 },
	{ 0, 725, 129 },
	{ 0, 725, 63 },
	{ 0, 725, 0 },
	{ 63, 725, 0 },
	{ 129, 725, 0 },
	{ 196, 725, 0 },
	{ 262, 725, 0 },
	{ 329, 725, 0 },
	{ 396, 725, 0 },
	{ 462, 725, 0 },
	{ 529, 725, 0 },
	{ 592, 725, 0 },
	{ 658, 725, 0 },
	{ 725, 725, 0 },
	{ 725, 658, 0 },
	{ 725, 592, 0 },
	{ 725, 529, 0 },
	{ 725, 462, 0 },
	{ 725, 396, 0 },
	{ 725, 329, 0 },
	{ 725, 262, 0 },
	{ 725, 196, 0 },
	{ 725, 129, 0 },
	{ 725, 63, 0 },
	{ 462, 0, 0 },
	{ 462, 0, 63 },
	{ 462, 0, 129 },
	{ 462, 0, 196 },
	{ 462, 0, 262 },
	{ 462, 0, 329 },
	{ 462, 0, 396 },
	{ 462, 0, 462 },
	{ 396, 0, 462 },
	{ 329, 0, 462 },
	{ 262, 0, 462 },
	{ 196, 0, 462 },
	{ 129, 0, 462 },
	{ 63, 0, 462 },
	{ 0, 0, 462 },
	{ 0, 63, 462 },
	{ 0, 129, 462 },
	{ 0, 196, 462 },
	{ 0, 262, 462 },
	{ 0, 329, 462 },
	{ 0, 396, 462 },
	{ 0, 462, 462 },
	{ 0, 462, 396 },
	{ 0, 462, 329 },
	{ 0, 462, 262 },
	{ 0, 462, 196 },
	{ 0, 462, 129 },
	{ 0, 462, 63 },
	{ 0, 462, 0 },
	{ 63, 462, 0 },
	{ 129, 462, 0 },
	{ 196, 462, 0 },
	{ 262, 462, 0 },
	{ 329, 462, 0 },
	{ 396, 462, 0 },
	{ 462, 462, 0 },
	{ 462, 396, 0 },
	{ 462, 329, 0 },
	{ 462, 262, 0 },
	{ 462, 196, 0 },
	{ 462, 129, 0 },
	{ 462, 63, 0 },
	{ 262, 0, 0 },
	{ 262, 0, 63 },
	{ 262, 0, 129 },
	{ 262, 0, 196 },
	{ 262, 0, 262 },
	{ 196, 0, 262 },
	{ 129, 0, 262 },
	{ 63, 0, 262 },
	{ 0, 0, 262 },
	{ 0, 63, 262 },
	{ 0, 129, 262 },
	{ 0, 196, 262 },
	{ 0, 262, 262 },
	{ 0, 262, 196 },
	{ 0, 262, 129 },
	{ 0, 262, 63 },
	{ 0, 262, 0 },
	{ 63, 262, 0 },
	{ 129, 262, 0 },
	{ 196, 262, 0 },
	{ 262, 262, 0 },
	{ 262, 196, 0 },
	{ 262, 129, 0 },
	{ 262, 63, 0 },
	{ 992, 992, 992 },
	{ 0, 0, 0 }
};

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static FILE *fp;

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

/* ------------------------------------------------------------------------- */

static void make_font_filename(char *dst, const char *src)
{
	unsigned char c;
	
	while ((c = *src++) != 0)
	{
		switch (c)
		{
		case ' ':
		case '(':
		case ')':
			c = '_';
			break;
		}
		*dst++ = c;
	}
	*dst = '\0';
}

/* ------------------------------------------------------------------------- */

static gboolean html_out_stylesheet(GString *out)
{
	g_string_append_printf(out, "<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\"%s\n", spdview_css_name, html_closer);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static gboolean html_out_javascript(GString *out)
{
	g_string_append_printf(out, "<script type=\"text/javascript\" src=\"%s\" charset=\"%s\"></script>\n", spdview_js_name, charset);
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void html_out_header(GString *out, const char *title, gboolean for_error)
{
	const char *html_lang = "en-US";
	
	g_string_append(out, "<!DOCTYPE html>\n");
	g_string_append_printf(out, "<html xml:lang=\"%s\" lang=\"%s\">\n", html_lang, html_lang);
	g_string_append_printf(out, "<!-- This file was automatically generated by %s version %s -->\n", gl_program_name, gl_program_version);
	g_string_append_printf(out, "<!-- Copyright \302\251 1991-2017 by Thorsten Otto -->\n");
	g_string_append(out, "<head>\n");
	g_string_append_printf(out, "<meta charset=\"%s\"%s\n", charset, html_closer);
	g_string_append_printf(out, "<meta name=\"GENERATOR\" content=\"%s %s\"%s\n", gl_program_name, gl_program_version, html_closer);
	if (title)
		g_string_append_printf(out, "<title>%s</title>\n", title);

	html_out_stylesheet(out);
	html_out_javascript(out);
	
	{
		g_string_append(out, "<!--[if lt IE 9]>\n");
		g_string_append(out, "<script src=\"http://html5shiv.googlecode.com/svn/trunk/html5.js\" type=\"text/javascript\"></script>\n");
		g_string_append(out, "<![endif]-->\n");
	}

	g_string_append(out, "</head>\n");
	g_string_append(out, "<body>\n");

	if (for_error)
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_error_note_style);
		g_string_append(out, "<p>\n");
	} else
	{
		g_string_append_printf(out, "<div class=\"%s\">\n", html_node_style);
	}
}

/* ------------------------------------------------------------------------- */

static void html_out_trailer(GString *out, gboolean for_error)
{
	if (for_error)
	{
		g_string_append(out, "</p>\n");
		g_string_append(out, "</div>\n");
	} else
	{
		g_string_append(out, "</div>\n");
	}
	
	g_string_append(out, "</body>\n");
	g_string_append(out, "</html>\n");
}

/* ------------------------------------------------------------------------- */
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
	
	va_start(v, str);
	vfprintf(errorfile, str, v);
	va_end(v);
}

/* ------------------------------------------------------------------------- */

void sp_open_bitmap(fix31 x_set_width, fix31 y_set_width, fix31 xorg, fix31 yorg, fix15 xsize, fix15 ysize)
{
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
		fprintf(errorfile, "char 0x%x (0x%x) wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
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
		fprintf(errorfile, "bbox & width mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.xmax - bb.xmin), bit_width);
	if ((bb.ymax - bb.ymin) != bit_height)
		fprintf(errorfile, "bbox & height mismatch 0x%x (0x%x) (%d vs %d)\n",
				char_index, char_id, (bb.ymax - bb.ymin), bit_height);
	if (bb.xmin != off_horz)
		fprintf(errorfile, "x min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.xmin, off_horz);
	if (bb.ymin != off_vert)
		fprintf(errorfile, "y min mismatch 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bb.ymin, off_vert);

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
		fprintf(errorfile, "width too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		fprintf(errorfile, "height too large 0x%x (0x%x) (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_height = MAX_BITS;
	}
	
	infos[char_index - first_char_index].width = bit_width;
	infos[char_index - first_char_index].height = bit_height;
	infos[char_index - first_char_index].off_horz = off_horz;
	infos[char_index - first_char_index].off_vert = off_vert;
	
	memset(framebuffer, 0, sizeof(framebuffer));
}

/* ------------------------------------------------------------------------- */

static int trunc = 0;

void sp_set_bitmap_bits(fix15 y, fix15 xbit1, fix15 xbit2)
{
	fix15 i;

	if (xbit1 > MAX_BITS)
	{
		fprintf(errorfile, "run wider than max bits -- truncated\n");
		xbit1 = MAX_BITS;
	}
	if (xbit2 > MAX_BITS)
	{
		fprintf(errorfile, "run wider than max bits -- truncated\n");
		xbit2 = MAX_BITS;
	}

	if (y >= bit_height)
	{
		fprintf(errorfile, "y value is larger than height 0x%x (0x%x) -- truncated\n", char_index, char_id);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		framebuffer[y][i] = vdi_maptab256[1];
	}
}

/* ------------------------------------------------------------------------- */

buff_t *sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset)
{
	if (fseek(fp, (long) file_offset, (int) 0))
	{
		fprintf(errorfile, "can't seek to char\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if ((num + cb_offset) > mincharsize)
	{
		fprintf(errorfile, "char buf overflow\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		fprintf(errorfile, "can't get char data\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	char_data.org = (ufix8 *) c_buffer + cb_offset;
	char_data.no_bytes = num;

	return &char_data;
}

/* ------------------------------------------------------------------------- */

static gboolean write_png(void)
{
	const charinfo *cinfo = &infos[char_index - first_char_index];
	int i;
	writepng_info *info;
	int rc;
	
	info = writepng_new();
	info->rowbytes = MAX_BITS;
	info->bpp = 8;
	info->image_data = &framebuffer[0][0];
	info->width = cinfo->width;
	info->height = cinfo->height;
	info->have_bg = vdi_maptab256[0];
	info->x_res = (x_res * 10000L) / 254;
	info->y_res = (y_res * 10000L) / 254;
	/*
	 * we cannot write a 0x0 image :(
	 */
	if (info->width == 0)
		info->width = 1;
	if (info->height == 0)
		info->height = 1;
	info->outfile = fopen(cinfo->local_filename, "wb");
	if (info->outfile == NULL)
	{
		rc = errno;
	} else
	{
		int num_colors = 256;
		
		info->num_palette = num_colors;
		for (i = 0; i < num_colors; i++)
		{
			int c;
			unsigned char pix;
			pix = vdi_maptab256[i];
			c = palette[i][0]; c = c * 255 / 1000;
			info->palette[pix].red = c;
			c = palette[i][1]; c = c * 255 / 1000;
			info->palette[pix].green = c;
			c = palette[i][2]; c = c * 255 / 1000;
			info->palette[pix].blue = c;
		}
		rc = writepng_output(info);
		fclose(info->outfile);
	}
	writepng_exit(info);
	return rc == 0;
}

/* ------------------------------------------------------------------------- */

void sp_close_bitmap(void)
{
	write_png();
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

static int cmp_info(const void *_a, const void *_b)
{
	const charinfo *a = (const charinfo *)_a;
	const charinfo *b = (const charinfo *)_b;
	return (int16_t)(a->char_id - b->char_id);
}

/* ------------------------------------------------------------------------- */

static void gen_hor_line(GString *body)
{
	int i;
	
	g_string_append(body, "<tr>\n");
	for (i = 0; i < (CHAR_COLUMNS * 2 + 1); i++)
		g_string_append(body, "<td class=\"horizontal_line\"></td>\n");
	g_string_append(body, "</tr>\n");
}

/* ------------------------------------------------------------------------- */

static gboolean gen_speedo_font(const char *filename, GString *body)
{
	gboolean decode_ok = TRUE;
	const ufix8 *key;
	uint16_t i, id, j;
	uint16_t num_ids;
	
	html_out_header(body, NULL, FALSE);

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
	if (num_chars == 0)
	{
		fprintf(errorfile, "no characters in font\n");
		return FALSE;
	}
	
	/* set up specs */
	/* Note that point size is in decipoints */
	specs.pfont = &font;
	/* XXX beware of overflow */
	specs.xxmult = point_size * x_res / 720 * (1L << 16);
	specs.xymult = 0L << 16;
	specs.xoffset = 0L << 16;
	specs.yxmult = 0L << 16;
	specs.yymult = point_size * y_res / 720 * (1L << 16);
	specs.yoffset = 0L << 16;
	switch (quality)
	{
	case 0:
		specs.flags = 0;
		break;
	case 1:
	default:
		specs.flags = MODE_SCREEN;
		break;
	case 2:
		specs.flags = MODE_2D;
		break;
	}

	chomp(fontname, (char *) (font_buffer + FH_FNTNM), sizeof(fontname));
	make_font_filename(fontfilename, fontname);
	
	if (!sp_set_specs(&specs))
	{
		decode_ok = FALSE;
	} else
	{
		time_t t;
		struct tm tm;
		char *basename;
		
		infos = g_new(charinfo, num_chars);
		if (infos == NULL)
			return FALSE;
		
		t = time(NULL);
		tm = *gmtime(&t);
		basename = g_strdup_printf("%s_%04d%02d%02d%02d%02d%02d_",
			fontfilename,
			tm.tm_year + 1900,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);
		
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			infos[i].char_index = char_index;
			infos[i].char_id = char_id;
			infos[i].local_filename = g_strdup_printf("%s/%schr%04x.png", output_dir, basename, char_id);
			infos[i].url = g_strdup_printf("%s/%schr%04x.png", output_url, basename, char_id);
			if (char_id)
			{
				if (!sp_make_char(char_index))
				{
					fprintf(errorfile, "can't make char %d (%x)\n", char_index, char_id);
				}
			}
		}
		
		qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
		
		num_ids = infos[num_chars - 1].char_id + 1;
		num_ids = ((num_ids + CHAR_COLUMNS - 1) / CHAR_COLUMNS) * CHAR_COLUMNS;
		
		g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
		i = 0;
		for (id = 0; id < num_ids; id += CHAR_COLUMNS)
		{
			gboolean defined[CHAR_COLUMNS];
			const char *klass;
			const char *img[CHAR_COLUMNS];
			int cell_width = 40;
			static char const vert_line[] = "<td class=\"vertical_line\" width=\"1\"></td>\n";
			
			if (id != 0 && (id % PAGE_SIZE) == 0)
				g_string_append(body, "<tr><td width=\"1\" height=\"10\"></td></tr>\n");

			if ((id % PAGE_SIZE) == 0)
				gen_hor_line(body);
			
			g_string_append(body, "<tr>\n");
			for (j = 0; j < CHAR_COLUMNS; j++)
			{
				defined[j] = FALSE;
				img[j] = NULL;
				if (i < num_chars && (id + j) == infos[i].char_id)
				{
					defined[j] = TRUE;
					if (infos[i].url != NULL)
						img[j] = infos[i].url;
					i++;
				}
				klass = defined[j] ? "spd_glyph_defined" : "spd_glyph_undefined";
				
				g_string_append(body, vert_line);
				
				g_string_append_printf(body,
					"<td class=\"%s\" width=\"%d\">%x</td>\n",
					klass,
					cell_width,
					id + j);
			}				
				
			g_string_append(body, vert_line);
			g_string_append(body, "</tr>\n");
			gen_hor_line(body);
				
			g_string_append(body, "<tr>\n");
			for (j = 0; j < CHAR_COLUMNS; j++)
			{
				g_string_append(body, vert_line);
				
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" width=\"%d\"><img src=\"%s\"></td>",
					cell_width,
					img[j] ? img[j] : "empty.png");
			}				
				
			g_string_append(body, vert_line);
			g_string_append(body, "</tr>\n");
			gen_hor_line(body);
		}
		if ((id % PAGE_SIZE) != 0)
			gen_hor_line(body);
		g_string_append(body, "</table>\n");

		for (i = 0; i < num_chars; i++)
		{
			g_free(infos[i].local_filename);
			g_free(infos[i].url);
		}
		g_free(infos);
		infos = NULL;
		g_free(basename);
	}

	html_out_trailer(body, FALSE);

	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static gboolean load_speedo_font(const char *filename, GString *body)
{
	gboolean ret;
	ufix8 tmp[16];
	ufix32 minbufsize;
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(errorfile, "%s: %s", filename, strerror(errno));
		return FALSE;
	}
	if (fread(tmp, sizeof(ufix8), 16, fp) != 16)
	{
		fclose(fp);
		fprintf(errorfile, "%s: read error\n", filename);
		return FALSE;
	}
	g_free(font_buffer);
	g_free(c_buffer);
	c_buffer = NULL;
	minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
	font_buffer = g_new(ufix8, minbufsize);
	if (font_buffer == NULL)
	{
		fclose(fp);
		fprintf(errorfile, "%s", strerror(errno));
		return FALSE;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(font_buffer, sizeof(ufix8), minbufsize, fp) != minbufsize)
	{
		fclose(fp);
		g_free(font_buffer);
		font_buffer = NULL;
		fprintf(errorfile, "%s: read error\n", filename);
		return FALSE;
	}

	mincharsize = read_2b(font_buffer + FH_CBFSZ);

	c_buffer = g_new(ufix8, mincharsize);
	if (!c_buffer)
	{
		fclose(fp);
		g_free(font_buffer);
		font_buffer = NULL;
		fprintf(errorfile, "%s", strerror(errno));
		return FALSE;
	}

	font.org = font_buffer;
	font.no_bytes = minbufsize;

	ret = gen_speedo_font(filename, body);
	fclose(fp);

	g_free(font_buffer);
	font_buffer = NULL;
	g_free(c_buffer);
	c_buffer = NULL;

	return ret;
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static gboolean uri_has_scheme(const char *uri)
{
	gboolean colon = FALSE;
	
	if (uri == NULL)
		return FALSE;
	while (*uri)
	{
		if (*uri == ':')
			colon = TRUE;
		else if (*uri == '/')
			return colon;
		uri++;
	}
	return colon;
}

/* ------------------------------------------------------------------------- */

static void html_out_response_header(FILE *out, unsigned long len)
{
	fprintf(out, "Content-Type: %s;charset=%s\015\012", "text/html", charset);
	fprintf(out, "Content-Length: %lu\015\012", len);
	fprintf(out, "Cache-Control: no-cache\015\012");
	fprintf(out, "\015\012");
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static size_t mycurl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct curl_parms *parms = (struct curl_parms *) userdata;
	
	if (size == 0 || nmemb == 0)
		return 0;
	if (parms->fp == NULL)
	{
		parms->fp = fopen(parms->filename, "wb");
		if (parms->fp == NULL)
			fprintf(errorfile, "%s: %s\n", parms->filename, strerror(errno));
	}
	if (parms->fp == NULL)
		return 0;
	return fwrite(ptr, size, nmemb, parms->fp);
}

/* ------------------------------------------------------------------------- */

static int mycurl_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userdata)
{
	struct curl_parms *parms = (struct curl_parms *) userdata;

	UNUSED(handle);
	UNUSED(parms);
	switch (type)
	{
	case CURLINFO_TEXT:
		fprintf(errorfile, "== Info: %s", data);
		if (size == 0 || data[size - 1] != '\n')
			fputc('\n', errorfile);
		break;
	case CURLINFO_HEADER_OUT:
		fprintf(errorfile, "=> Send header %ld\n", (long)size);
		fwrite(data, 1, size, errorfile);
		break;
	case CURLINFO_DATA_OUT:
		fprintf(errorfile, "=> Send data %ld\n", (long)size);
		break;
	case CURLINFO_SSL_DATA_OUT:
		fprintf(errorfile, "=> Send SSL data %ld\n", (long)size);
		break;
	case CURLINFO_HEADER_IN:
		fprintf(errorfile, "<= Recv header %ld\n", (long)size);
		fwrite(data, 1, size, errorfile);
		break;
	case CURLINFO_DATA_IN:
		fprintf(errorfile, "<= Recv data %ld\n", (long)size);
		break;
	case CURLINFO_SSL_DATA_IN:
		fprintf(errorfile, "<= Recv SSL data %ld\n", (long)size);
		break;
	case CURLINFO_END:
	default:
		break;
 	}
	return 0;
}

/* ------------------------------------------------------------------------- */

static const char *currdate(void)
{
	struct tm *tm;
	static char buf[40];
	time_t t;
	t = time(NULL);
	tm = localtime(&t);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
	return buf;
}
	
/* ------------------------------------------------------------------------- */

static char *curl_download(CURL *curl, GString *body, const char *filename)
{
	char *local_filename;
	struct curl_parms parms;
	struct stat st;
	long unmet;
	long respcode;
	CURLcode curlcode;
	double size;
	char err[CURL_ERROR_SIZE];
	char *content_type;

	curl_easy_setopt(curl, CURLOPT_URL, filename);
	curl_easy_setopt(curl, CURLOPT_REFERER, filename);
	local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
	parms.filename = local_filename;
	parms.fp = NULL;
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mycurl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &parms);
	curl_easy_setopt(curl, CURLOPT_STDERR, errorfile);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS, ALLOWED_PROTOS);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, (long)1);
	curl_easy_setopt(curl, CURLOPT_FILETIME, (long)1);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, mycurl_trace);
	curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &parms);
	*err = 0;
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	
	/* set this to 1 to activate debug code above */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, (long)0);

	if (stat(local_filename, &st) == 0)
	{
		curlcode = curl_easy_setopt(curl, CURLOPT_TIMECONDITION, (long)CURL_TIMECOND_IFMODSINCE);
		curlcode = curl_easy_setopt(curl, CURLOPT_TIMEVALUE, (long)st.st_mtime);
	}
	
	/*
	 * TODO: reject attempts to connect to local addresses
	 */
	curlcode = curl_easy_perform(curl);
	
	respcode = 0;
	unmet = -1;
	size = 0;
	content_type = NULL;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respcode);
	curl_easy_getinfo(curl, CURLINFO_CONDITION_UNMET, &unmet);
	curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &size);
	curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
	fprintf(errorfile, "%s: GET from %s, url=%s, curl=%d, resp=%ld, size=%ld, content=%s\n", currdate(), fixnull(cgiRemoteHost), filename, curlcode, respcode, (long)size, printnull(content_type));
	
	if (parms.fp)
	{
		fclose(parms.fp);
		parms.fp = NULL;
	}
	
	if (curlcode != CURLE_OK)
	{
		html_out_header(body, err, TRUE);
		g_string_append_printf(body, "%s:\n%s", _("Download error"), err);
		html_out_trailer(body, TRUE);
		unlink(local_filename);
		g_free(local_filename);
		local_filename = NULL;
	} else if ((respcode != 200 && respcode != 304) ||
		(respcode == 200 && (content_type == NULL || strcmp(content_type, "text/plain") == 0)))
	{
		/* most likely the downloaded data will contain the error page */
		parms.fp = fopen(local_filename, "rb");
		if (parms.fp != NULL)
		{
			size_t nread;
			
			while ((nread = fread(err, 1, sizeof(err), parms.fp)) > 0)
				g_string_append_len(body, err, nread);
			fclose(parms.fp);
		}
		unlink(local_filename);
		g_free(local_filename);
		local_filename = NULL;
	} else
	{
		long ft = -1;
		if (curl_easy_getinfo(curl, CURLINFO_FILETIME, &ft) == CURLE_OK && ft != -1)
		{
			struct utimbuf ut;
			ut.actime = ut.modtime = ft;
			utime(local_filename, &ut);
		}
	}
	
	return local_filename;
}

/* ------------------------------------------------------------------------- */

static void write_strout(GString *s, FILE *outfp)
{
	if (force_crlf)
	{
		const char *txt = s->str;
		while (*txt)
		{
			if (*txt == '\n')
			{
				fputc('\015', outfp);
				fputc('\012', outfp);
			}  else
			{
				fputc(*txt, outfp);
			}
			txt++;
		}
	} else
	{
		fwrite(s->str, 1, s->len, outfp);
	}
}

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

int main(void)
{
	int retval = EXIT_SUCCESS;
	FILE *out = stdout;
	GString *body;
	CURL *curl = NULL;
	char *lang;
	char *val;
	
	errorfile = stderr;
	
	body = g_string_new(NULL);
	cgiInit(body);

	{
		char *dir = spd_path_get_dirname(cgiScriptFilename);
		char *cache_dir = g_build_filename(dir, cgi_cachedir, NULL);
		DIR *dp;
		struct dirent *e;
		
		output_dir = g_build_filename(cache_dir, cgiRemoteAddr, NULL);
		output_url = g_build_filename(cgi_cachedir, cgiRemoteAddr, NULL);

		if (mkdir(cache_dir, 0750) < 0 && errno != EEXIST)
			fprintf(errorfile, "%s: %s\n", cache_dir, strerror(errno));
		if (mkdir(output_dir, 0750) < 0 && errno != EEXIST)
			fprintf(errorfile, "%s: %s\n", output_dir, strerror(errno));
		
		/*
		 * clean up from previous run(s),
		 * otherwise lots of files pile up there
		 */
		dp = opendir(output_dir);
		if (dp != NULL)
		{
			while ((e = readdir(dp)) != NULL)
			{
				char *f;
				const char *p;
				
				if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
					continue;
				if ((p = strrchr(e->d_name, '.')) != NULL && strcmp(p, ".png") == 0)
				{
					f = g_build_filename(output_dir, e->d_name, NULL);
					unlink(f);
					g_free(f);
				}
			}
			closedir(dp);
		}
		
		g_free(cache_dir);
		g_free(dir);
	}
	
	if (cgiScriptName)
		cgi_scriptname = cgiScriptName;
	
	if ((val = cgiFormString("points")) != NULL)
	{
		point_size = strtol(val, NULL, 10);
		g_free(val);
		if (point_size < 40 || point_size > 10000)
			point_size = 120;
	}
	if ((val = cgiFormString("quality")) != NULL)
	{
		quality = (int)strtol(val, NULL, 10);
		g_free(val);
	}

	lang = cgiFormString("lang");
	
	if (g_ascii_strcasecmp(cgiRequestMethod, "GET") == 0)
	{
		char *url = cgiFormString("url");
		char *filename = g_strdup(url);
		char *scheme = empty(filename) ? g_strdup("undefined") : uri_has_scheme(filename) ? g_strndup(filename, strchr(filename, ':') - filename) : g_strdup("file");
		
		if (filename && filename[0] == '/')
		{
			html_referer_url = filename;
			filename = g_strconcat(cgiDocumentRoot, filename, NULL);
		} else if (empty(xbasename(filename)) || (g_ascii_strcasecmp(scheme, "file") == 0))
		{
			/*
			 * disallow file URIs, they would resolve to local files on the WEB server
			 */
			html_out_header(body, _("403 Forbidden"), TRUE);
			g_string_append_printf(body,
				_("Sorry, this type of\n"
				  "<a href=\"http://www.w3.org/Addressing/\">URL</a>\n"
				  "<a href=\"http://www.iana.org/assignments/uri-schemes.html\">scheme</a>\n"
				  "(<q>%s</q>) is not\n"
				  "supported by this service. Please check that you entered the URL correctly.\n"),
				scheme);
			html_out_trailer(body, TRUE);
			g_free(filename);
			filename = NULL;
		} else
		{
			if ((curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK ||
				(curl = curl_easy_init()) == NULL))
			{
				html_out_header(body, _("500 Internal Server Error"), TRUE);
				g_string_append(body, _("could not initialize curl\n"));
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				char *local_filename;
				
				html_referer_url = g_strdup(filename);
				local_filename = curl_download(curl, body, filename);
				g_free(filename);
				filename = local_filename;
			}
		}
		if (filename && retval == EXIT_SUCCESS)
		{
			if (load_speedo_font(filename, body) == FALSE)
			{
				retval = EXIT_FAILURE;
			}
			g_free(filename);
		}
		g_free(scheme);
		g_free(html_referer_url);
		html_referer_url = NULL;
		
		g_free(url);
	} else if (g_ascii_strcasecmp(cgiRequestMethod, "POST") == 0 ||
		g_ascii_strcasecmp(cgiRequestMethod, "BOTH") == 0)
	{
		char *filename;
		int len;
		
		g_string_truncate(body, 0);
		filename = cgiFormFileName("file", &len);
		if (filename == NULL || len == 0)
		{
			const char *scheme = "undefined";
			html_out_header(body, _("403 Forbidden"), TRUE);
			g_string_append_printf(body,
				_("Sorry, this type of\n"
				  "<a href=\"http://www.w3.org/Addressing/\">URL</a>\n"
				  "<a href=\"http://www.iana.org/assignments/uri-schemes.html\">scheme</a>\n"
				  "(<q>%s</q>) is not\n"
				  "supported by this service. Please check that you entered the URL correctly.\n"),
				scheme);
			html_out_trailer(body, TRUE);
		} else
		{
			FILE *fp = NULL;
			char *local_filename;
			const char *data;
			
			if (*filename == '\0')
			{
				g_free(filename);
#if defined(HAVE_MKSTEMPS)
				{
				int fd;
				filename = g_strdup("tmpfile.XXXXXX.spd");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fd = mkstemps(local_filename, 4);
				if (fd > 0)
					fp = fdopen(fd, "wb");
				}
#elif defined(HAVE_MKSTEMP)
				{
				int fd;
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fd = mkstemp(local_filename);
				if (fd > 0)
					fp = fdopen(fd, "wb");
				}
#else
				filename = g_strdup("tmpfile.spd.XXXXXX");
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				mktemp(local_filename);
				fp = fopen(local_filename, "wb");
#endif
			} else
			{
				local_filename = g_build_filename(output_dir, xbasename(filename), NULL);
				fp = fopen(local_filename, "wb");
			}

			fprintf(errorfile, "%s: POST from %s, file=%s, size=%d\n", currdate(), fixnull(cgiRemoteHost), xbasename(filename), len);

			if (fp == NULL)
			{
				const char *err = strerror(errno);
				fprintf(errorfile, "%s: %s\n", local_filename, err);
				html_out_header(body, _("404 Not Found"), TRUE);
				g_string_append_printf(body, "%s: %s\n", xbasename(filename), err);
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				data = cgiFormFileData("file", &len);
				fwrite(data, 1, len, fp);
				fclose(fp);
				html_referer_url = g_strdup(filename);
				if (load_speedo_font(local_filename, body) == FALSE)
				{
					retval = EXIT_FAILURE;
				}
			}
			g_free(local_filename);
		}
		g_free(filename);
		g_free(html_referer_url);
		html_referer_url = NULL;
	}
	
	html_out_response_header(out, body->len);
	cgiExit();
	write_strout(body, out);
	g_string_free(body, TRUE);
	
	g_free(lang);
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	
	return retval;
}
