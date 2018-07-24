#include "linux/libcwrap.h"
#include "defs.h"
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include "cgic.h"
#include "speedo.h"
#include "writepng.h"
#include "vdimaps.h"
#include "bics2uni.h"
#include "cgiutil.h"
#include "ucd.h"
#include "version.h"

char const gl_program_name[] = "spdview.cgi";
const char *cgi_scriptname = "spdview.cgi";
char const html_nav_load_href[] = "index.php";

#define _(x) x

#define DEBUG 0

#define	MAX_BITS	1024
#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

static unsigned char framebuffer[MAX_BITS][MAX_BITS];
static uint16_t char_index, char_id;
static buff_t font;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static uint16_t mincharsize;
static fix15 bit_width, bit_height;
static char fontname[70 + 1];
static char fontfilename[70 + 1];
static uint16_t first_char_index;
static uint16_t num_chars;
static glyphinfo_t font_bb;

static long point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 1;
static gboolean optimize_output = TRUE;

static specs_t specs;

#define HAVE_MKSTEMPS

typedef struct {
	uint16_t char_index;
	uint16_t char_id;
	char *local_filename;
	char *url;
	glyphinfo_t bbox;
} charinfo;
static charinfo *infos;

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

static FILE *fp;

static void chomp(char *dst, const char *src, size_t maxlen)
{
	size_t len;
	
	strncpy(dst, src, maxlen - 1);
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
/*
 * Reads 1-byte field from font buffer 
 */
#define read_1b(pointer) (*(pointer))

/* ------------------------------------------------------------------------- */

static fix15 read_2b(const ufix8 *ptr)
{
	fix15 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

static fix31 read_4b(const ufix8 *ptr)
{
	fix31 tmp;

	tmp = *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr++;
	tmp = (tmp << 8) + *ptr;
	return tmp;
}

/* ------------------------------------------------------------------------- */

#if 0
static char *html_cgi_params(void)
{
	return g_strdup_printf("%s%s&amp;quality=%d&amp;points=%ld",
		hidemenu ? "&amp;hidemenu=1" : "",
		cgi_cached ? "&amp;cached=1" : "",
		quality,
		point_size);
}
#endif
	
/* ------------------------------------------------------------------------- */

static GString *get_font_info(const unsigned char *font)
{
	GString *out = g_string_new(NULL);
	char *str;
	
	g_string_append_printf(out, "Font size: %ld<br />\n", (long)read_4b(font + FH_FNTSZ));
	g_string_append_printf(out, "Minimum font buffer size: %ld<br />\n", (long)read_4b(font + FH_FBFSZ));
	g_string_append_printf(out, "Minimum character buffer size: %u<br />\n", read_2b(font + FH_CBFSZ));
	g_string_append_printf(out, "Header size: %u<br />\n", read_2b(font + FH_HEDSZ));
	g_string_append_printf(out, "Font ID: %u<br />\n", read_2b(font + FH_FNTID));
	g_string_append_printf(out, "Font version number: %u<br />\n", read_2b(font + FH_SFVNR));
	str = html_quote_name((const char *)font + FH_FNTNM, QUOTE_UNICODE, 70);
	g_string_append_printf(out, "Font full name: %s<br />\n", str);
	g_free(str);
	str = html_quote_name((const char *)font + FH_SFNTN, QUOTE_UNICODE, 32);
	g_string_append_printf(out, "Short font name: %s<br />\n", str);
	g_free(str);
	str = html_quote_name((const char *)font + FH_SFACN, QUOTE_UNICODE, 16);
	g_string_append_printf(out, "Short face name: %s<br />\n", str);
	g_free(str);
	str = html_quote_name((const char *)font + FH_FNTFM, QUOTE_UNICODE, 14);
	g_string_append_printf(out, "Font form: %s<br />\n", str);
	g_free(str);
	str = html_quote_name((const char *)font + FH_MDATE, QUOTE_UNICODE, 10);
	g_string_append_printf(out, "Manufacturing date: %s<br />\n", str);
	g_free(str);
	str = html_quote_name((const char *)font + FH_LAYNM, QUOTE_UNICODE, 70);
	g_string_append_printf(out, "Layout name: %s<br />\n", str);
	g_free(str);
	g_string_append_printf(out, "Number of chars in layout: %u<br />\n", read_2b(font + FH_NCHRL));
	g_string_append_printf(out, "Total Number of chars in font: %u<br />\n", read_2b(font + FH_NCHRF));
	g_string_append_printf(out, "Index of first character: %u<br />\n", read_2b(font + FH_FCHRF));
	g_string_append_printf(out, "Number of Kerning Tracks: %u<br />\n", read_2b(font + FH_NKTKS));
	g_string_append_printf(out, "Number of Kerning Pairs: %u<br />\n", read_2b(font + FH_NKPRS));
	g_string_append_printf(out, "Flags: 0x%x<br />\n", read_1b(font + FH_FLAGS));
	g_string_append_printf(out, "Classification Flags: 0x%x<br />\n", read_1b(font + FH_CLFGS));
	g_string_append_printf(out, "Family Classification: 0x%x<br />\n", read_1b(font + FH_FAMCL));
	g_string_append_printf(out, "Font form classification: 0x%x<br />\n", read_1b(font + FH_FRMCL));
	g_string_append_printf(out, "Italic angle: %d<br />\n", read_2b(font + FH_ITANG));
	g_string_append_printf(out, "Number of ORUs per em: %d<br />\n", read_2b(font + FH_ORUPM));
	g_string_append_printf(out, "Width of Wordspace: %d<br />\n", read_2b(font + FH_WDWTH));
	g_string_append_printf(out, "Width of Emspace: %d<br />\n", read_2b(font + FH_EMWTH));
	g_string_append_printf(out, "Width of Enspace: %d<br />\n", read_2b(font + FH_ENWTH));
	g_string_append_printf(out, "Width of Thinspace: %d<br />\n", read_2b(font + FH_TNWTH));
	g_string_append_printf(out, "Width of Figspace: %d<br />\n", read_2b(font + FH_FGWTH));
	g_string_append_printf(out, "Font-wide bounding box: %d %d %d %d<br />\n", read_2b(font + FH_FXMIN), read_2b(font + FH_FYMIN), read_2b(font + FH_FXMAX), read_2b(font + FH_FYMAX));
	g_string_append_printf(out, "Underline position: %d<br />\n", read_2b(font + FH_ULPOS));
	g_string_append_printf(out, "Underline thickness: %d<br />\n", read_2b(font + FH_ULTHK));
	g_string_append_printf(out, "Small caps: %d<br />\n", read_2b(font + FH_SMCTR));
	g_string_append_printf(out, "Display Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_DPSTR), (double) read_2b(font + FH_DPSTR + 2) / 4096.0, (double) read_2b(font + FH_DPSTR + 4) / 4096.0);
	g_string_append_printf(out, "Footnote Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_FNSTR), (double) read_2b(font + FH_FNSTR + 2) / 4096.0, (double) read_2b(font + FH_FNSTR + 4) / 4096.0);
	g_string_append_printf(out, "Alpha Superiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_ALSTR), (double) read_2b(font + FH_ALSTR + 2) / 4096.0, (double) read_2b(font + FH_ALSTR + 4) / 4096.0);
	g_string_append_printf(out, "Chemical Inferiors: %d %7.2f %7.2f<br />\n", read_2b(font + FH_CMITR), (double) read_2b(font + FH_CMITR + 2) / 4096.0, (double) read_2b(font + FH_CMITR + 4) / 4096.0);
	g_string_append_printf(out, "Small Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_SNMTR), (double) read_2b(font + FH_SNMTR + 2) / 4096.0, (double) read_2b(font + FH_SNMTR + 4) / 4096.0);
	g_string_append_printf(out, "Small Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_SDNTR), (double) read_2b(font + FH_SDNTR + 2) / 4096.0, (double) read_2b(font + FH_SDNTR + 4) / 4096.0);
	g_string_append_printf(out, "Medium Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_MNMTR), (double) read_2b(font + FH_MNMTR + 2) / 4096.0, (double) read_2b(font + FH_MNMTR + 4) / 4096.0);
	g_string_append_printf(out, "Medium Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_MDNTR), (double) read_2b(font + FH_MDNTR + 2) / 4096.0, (double) read_2b(font + FH_MDNTR + 4) / 4096.0);
	g_string_append_printf(out, "Large Numerators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_LNMTR), (double) read_2b(font + FH_LNMTR + 2) / 4096.0, (double) read_2b(font + FH_LNMTR + 4) / 4096.0);
	g_string_append_printf(out, "Large Denominators: %d %7.2f %7.2f<br />\n", read_2b(font + FH_LDNTR), (double) read_2b(font + FH_LDNTR + 2) / 4096.0, (double) read_2b(font + FH_LDNTR + 4) / 4096.0);

	return out;
}

/* ------------------------------------------------------------------------- */

/*
 * Called by Speedo character generator to report an error.
 */
void sp_write_error(const char *str, ...)
{
	va_list v;
	
	va_start(v, str);
	g_string_append_vprintf(errorout, str, v);
	g_string_append(errorout, "\n");
	va_end(v);
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
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	width = sp_get_char_width(char_index);
	pix_width = width * (specs.xxmult / 65536L) + ((ufix32) width * ((ufix32) specs.xxmult & 0xffff)) / 65536L;
	pix_width /= 65536L;

	width = (pix_width * 7200L) / (point_size * y_res);

	sp_get_char_bbox(char_index, &bb, FALSE);
	bb.xmin >>= 16;
	bb.ymin >>= 16;
	bb.xmax >>= 16;
	bb.ymax >>= 16;

#if DEBUG
	{
		fix15 off_horz;
		fix15 off_vert;
		
		off_horz = (fix15) ((xorg + 32768L) >> 16);
		off_vert = (fix15) ((yorg + 32768L) >> 16);

		if (((bb.xmax - bb.xmin)) != bit_width)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & width mismatch (%ld vs %d)\n",
					char_index, char_id, (unsigned long)(bb.xmax - bb.xmin), bit_width);
		if (((bb.ymax - bb.ymin)) != bit_height)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & height mismatch (%ld vs %d)\n",
					char_index, char_id, (unsigned long)(bb.ymax - bb.ymin), bit_height);
		if ((bb.xmin) != off_horz)
			g_string_append_printf(errorout, "char 0x%x (0x%x): x min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.xmin, off_horz);
		if ((bb.ymin) != off_vert)
			g_string_append_printf(errorout, "char 0x%x (0x%x): y min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.ymin, off_vert);
	}
#endif

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
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): width too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): height too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
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
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bit1 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit1, bit_width);
		xbit1 = MAX_BITS;
		trunc = 1;
	}
	if (xbit2 < 0 || xbit2 > bit_width)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): bit2 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit2, bit_width);
		xbit2 = MAX_BITS;
		trunc = 1;
	}

	if (y < 0 || y >= bit_height)
	{
		if (debug)
			g_string_append_printf(errorout, "char 0x%x (0x%x): y value %d is larger than height %u -- truncated\n", char_index, char_id, y, bit_height);
		trunc = 1;
		return;
	}

	for (i = xbit1; i < xbit2; i++)
	{
		framebuffer[y][i] = vdi_maptab256[1];
	}
}

/* ------------------------------------------------------------------------- */

#if INCL_LCD
boolean sp_load_char_data(long file_offset, fix15 num, fix15 cb_offset, buff_t *char_data)
{
	if (fseek(fp, file_offset, SEEK_SET))
	{
		g_string_append_printf(errorout, "%x (%x): can't seek to char at %ld\n", char_index, char_id, file_offset);
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if ((num + cb_offset) > mincharsize)
	{
		g_string_append_printf(errorout, "char buf overflow\n");
		char_data->org = c_buffer;
		char_data->no_bytes = 0;
		return FALSE;
	}
	if ((long)fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		g_string_append_printf(errorout, "can't get char data\n");
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

static gboolean write_png(void)
{
	const charinfo *c = &infos[char_id];
	int i;
	writepng_info *info;
	int rc;
	
	info = writepng_new();
	info->rowbytes = MAX_BITS;
	info->bpp = 8;
	info->image_data = &framebuffer[0][0];
	info->width = c->bbox.width;
	info->height = c->bbox.height;
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
	info->outfile = fopen(c->local_filename, "wb");
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

#define TRANS_X(x) (double)(x) / 65536.0 - font_bb.lbearing
#define TRANS_Y(y) (double)(((fix31) sp_globals.ymax << sp_globals.poshift) - (y)) / 65536.0 + (font_bb.ascent - c->bbox.ascent)

static FILE *svg_fp;

void sp_open_outline(fix31 x_set_width, fix31 y_set_width, fix31 xmin, fix31 xmax, fix31 ymin, fix31 ymax)
{
	const charinfo *c = &infos[char_id];

	if (svg_fp)
		fclose(svg_fp);
	svg_fp = fopen(c->local_filename, "wb");
	if (svg_fp == NULL)
		return;
	UNUSED(x_set_width);
	UNUSED(y_set_width);
	UNUSED(xmin);
	UNUSED(xmax);
	UNUSED(ymin);
	UNUSED(ymax);
	fprintf(svg_fp, "<!-- bbox %.2f %.2f %.2f %.2f -->\n",
		((fix31) sp_globals.xmin << sp_globals.poshift) / 65536.0,
		((fix31) sp_globals.ymin << sp_globals.poshift) / 65536.0,
		((fix31) sp_globals.xmax << sp_globals.poshift) / 65536.0,
		((fix31) sp_globals.ymax << sp_globals.poshift) / 65536.0);
	fprintf(svg_fp, "<svg xmlns=\"http://www.w3.org/2000/svg\" id=\"chr%04x\" viewBox=\"0 0 %d %d\">\n",
		c->char_id,
		font_bb.width, font_bb.height);
}

/* ------------------------------------------------------------------------- */

void sp_start_sub_char(void)
{
	if (svg_fp == NULL)
		return;
}

/* ------------------------------------------------------------------------- */

void sp_end_sub_char(void)
{
	if (svg_fp == NULL)
		return;
}

/* ------------------------------------------------------------------------- */

void sp_start_contour(fix31 x, fix31 y, boolean outside)
{
	const charinfo *c = &infos[char_id];

	if (svg_fp == NULL)
		return;
	fprintf(svg_fp, "<path style=\"fill:%s;fill-opacity:1;stroke:currentColor\"\n   d=\"", outside ? "none" : "none");
	fprintf(svg_fp, "M %.2f %.2f", TRANS_X(x), TRANS_Y(y));
}

/* ------------------------------------------------------------------------- */

void sp_curve_to(fix31 x1, fix31 y1, fix31 x2, fix31 y2, fix31 x3, fix31 y3)
{
	const charinfo *c = &infos[char_id];

	if (svg_fp == NULL)
		return;
	fprintf(svg_fp, " M %.2f %.2f", TRANS_X(x1), TRANS_Y(y1));
	fprintf(svg_fp, " Q %.2f %.2f %.2f %.2f", TRANS_X(x2), TRANS_Y(y2), TRANS_X(x3), TRANS_Y(y3));
}

/* ------------------------------------------------------------------------- */

void sp_line_to(fix31 x1, fix31 y1)
{
	const charinfo *c = &infos[char_id];

	if (svg_fp == NULL)
		return;
	fprintf(svg_fp, " L %.2f %.2f", TRANS_X(x1), TRANS_Y(y1));
}

/* ------------------------------------------------------------------------- */

void sp_close_contour(void)
{
	if (svg_fp == NULL)
		return;
	fprintf(svg_fp, "\" />\n");
}

/* ------------------------------------------------------------------------- */

void sp_close_outline(void)
{
	if (svg_fp == NULL)
		return;
	fprintf(svg_fp, "</svg>\n");
	fclose(svg_fp);
	svg_fp = NULL;
}

#undef TRANS_Y

#endif

/* ------------------------------------------------------------------------- */

#if 0
static int cmp_info(const void *_a, const void *_b)
{
	const charinfo *a = (const charinfo *)_a;
	const charinfo *b = (const charinfo *)_b;
	return (int16_t)(a->char_id - b->char_id);
}
#endif

/* ------------------------------------------------------------------------- */

static void gen_hor_line(GString *body, int columns)
{
	int i;
	
	g_string_append(body, "<tr>\n");
	for (i = 0; i < columns; i++)
	{
		g_string_append(body, "<td class=\"horizontal_line\" style=\"min-width: 1px;\"></td>\n");
		g_string_append(body, "<td class=\"horizontal_line\"></td>\n");
	}
	g_string_append(body, "<td class=\"horizontal_line\" style=\"min-width: 1px;\"></td>\n");
	g_string_append(body, "</tr>\n");
}

/* ------------------------------------------------------------------------- */

static void update_bbox(charinfo *c, glyphinfo_t *box)
{
	bbox_t bb;
	bbox_t bb2;
	
	sp_get_char_bbox(c->char_index, &bb, FALSE);
	sp_get_char_bbox(c->char_index, &bb2, TRUE);
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
	if (specs.output_mode == MODE_OUTLINE)
	{
		c->bbox.lbearing = (bb2.xmin + 32768L) >> 16;
		c->bbox.off_vert = (bb2.ymin + ((bb2.ymax - bb2.ymin) - (bb.ymax - bb.ymin)) / 2 + 32768L) >> 16;
	} else
	{
		c->bbox.lbearing = (bb2.xmin + 32768L) >> 16;
		c->bbox.off_vert = (bb2.ymin - (bb.ymax - bb.ymin) + (bb2.ymax - bb2.ymin) + 3932L) >> 16;
	}
	box->lbearing = MIN(box->lbearing, c->bbox.lbearing);
	c->bbox.rbearing = c->bbox.width + c->bbox.lbearing;
	box->rbearing = MAX(box->rbearing, c->bbox.rbearing);
	c->bbox.ascent = c->bbox.height + c->bbox.off_vert;
	c->bbox.descent = c->bbox.height - c->bbox.ascent;
	box->width = MAX(box->width, c->bbox.width);
	box->height = MAX(box->height, c->bbox.height);
	box->ascent = MAX(box->ascent, c->bbox.ascent);
	box->descent = MAX(box->descent, c->bbox.descent);
}

/* ------------------------------------------------------------------------- */

static gboolean gen_speedo_font(GString *body)
{
	gboolean decode_ok = TRUE;
	time_t t;
	struct tm tm;
	char *basename;
	int columns;
	const ufix8 *key;
	uint16_t i, id, j;
	uint16_t num_ids;
	GString *font_info;
	size_t page_start;
	size_t line_start;
	gboolean any_defined_page, any_defined_line;
	
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
		g_string_append(errorout, "no characters in font\n");
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
	specs.flags = 0;
	switch (quality)
	{
	case 0:
		specs.output_mode = MODE_BLACK;
		break;
	case 1:
	default:
		specs.output_mode = MODE_SCREEN;
		break;
	case 2:
#if INCL_OUTLINE
		specs.output_mode = MODE_OUTLINE;
#else
		specs.output_mode = MODE_2D;
#endif
		break;
	case 3:
		specs.output_mode = MODE_2D;
		break;
	}

	chomp(fontname, (char *) (font_buffer + FH_FNTNM), sizeof(fontname));
	make_font_filename(fontfilename, fontname);
	font_info = get_font_info(font_buffer);
	html_out_header(body, NULL, font_info, fontname, FALSE);
	g_string_free(font_info, TRUE);
	
	if (!sp_set_specs(&specs, &font))
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
		g_string_append(errorout, "no characters in font\n");
		return FALSE;
	} 
#if 0
	num_ids = sp_get_char_id(num_chars + first_char_index - 1) + 1;
	num_ids = ((num_ids + CHAR_COLUMNS - 1) / CHAR_COLUMNS) * CHAR_COLUMNS;
#endif
	
	infos = g_new(charinfo, num_ids);
	if (infos == NULL)
	{
		g_string_append_printf(errorout, "%s\n", strerror(errno));
		return FALSE;
	}
	
	for (i = 0; i < num_ids; i++)
	{
		infos[i].char_index = 0;
		infos[i].char_id = UNDEFINED;
		infos[i].local_filename = NULL;
		infos[i].url = NULL;
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
					if (debug)
						g_string_append_printf(errorout, "char 0x%x (0x%x) already defined\n", char_index, char_id);
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
		if (debug)
		{
			g_string_append_printf(errorout, "max height %d max width %d average width %d\n", max_bb.height, max_bb.width, average_width);
		}
		
		max_bb.height = max_bb.ascent + max_bb.descent;
		max_bb.width = max_bb.rbearing - max_bb.lbearing;

		if (debug)
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
			g_string_append_printf(errorout, "bbox: %d %d ascent %d descent %d lb %d rb %d\n",
				max_bb.width, max_bb.height,
				max_bb.ascent, max_bb.descent,
				max_bb.lbearing, max_bb.rbearing);
			g_string_append_printf(errorout, "bbox (header): %d %d %d %d; %ld %ld %ld %ld\n",
				xmin, ymin,
				xmax, ymax,
				xmin * pixel_size / sp_globals.orus_per_em,
				ymin * pixel_size / sp_globals.orus_per_em,
				xmax * pixel_size / sp_globals.orus_per_em,
				ymax * pixel_size / sp_globals.orus_per_em);
		}
		
		font_bb = max_bb;
	}
	
	for (i = 0; i < num_chars; i++)
	{
		char_index = i + first_char_index;
		char_id = sp_get_char_id(char_index);
		if (char_id != SP_UNDEFINED && char_id != UNDEFINED)
		{
			const char *ext = specs.output_mode == MODE_OUTLINE ? ".svg" : ".png";
			infos[char_id].local_filename = g_strdup_printf("%s/%schr%04x%s", output_dir, basename, char_id, ext);
			infos[char_id].url = g_strdup_printf("%s/%schr%04x%s", output_url, basename, char_id, ext);
			if (!sp_make_char(char_index))
			{
				g_string_append_printf(errorout, "can't make char 0x%x (0x%x)\n", char_index, char_id);
			}
		}
	}
	
#if 0
	qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
#endif
	
	page_start = body->len;
	any_defined_page = FALSE;
	g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
	for (id = 0; id < num_ids; id += CHAR_COLUMNS)
	{
		gboolean defined[CHAR_COLUMNS];
		const char *klass;
		const char *img[CHAR_COLUMNS];
		static char const vert_line[] = "<td class=\"vertical_line\"></td>\n";
		
		if (id != 0 && (id % PAGE_SIZE) == 0)
		{
			if (optimize_output && !any_defined_page)
			{
				g_string_truncate(body, page_start);
			}
			page_start = body->len;
			any_defined_page = FALSE;
			g_string_append(body, "<tr><td width=\"1\" height=\"10\" style=\"padding: 0px; margin:0px;\"></td></tr>\n");
		}
		
		columns = CHAR_COLUMNS;
		if ((id + columns) > num_ids)
			columns = num_ids - id;
		
		if ((id % PAGE_SIZE) == 0)
			gen_hor_line(body, columns);
		
		line_start = body->len;
		any_defined_line = FALSE;
		g_string_append(body, "<tr>\n");
		for (j = 0; j < columns; j++)
		{
			uint16_t char_id = id + j;
			charinfo *c = &infos[char_id];
			
			defined[j] = FALSE;
			img[j] = NULL;
			if (c->char_id != UNDEFINED)
			{
				defined[j] = TRUE;
				any_defined_line |= TRUE;
				any_defined_page |= TRUE;
				if (c->url != NULL)
					img[j] = c->url;
			}
			klass = defined[j] ? "spd_glyph_defined" : "spd_glyph_undefined";
			
			g_string_append(body, vert_line);
			
			g_string_append_printf(body,
				"<td class=\"%s\">%x</td>\n",
				klass,
				id + j);
		}				
			
		g_string_append(body, vert_line);
		g_string_append(body, "</tr>\n");
		gen_hor_line(body, columns);
			
		g_string_append(body, "<tr>\n");
		for (j = 0; j < columns; j++)
		{
			uint16_t char_id = id + j;
			charinfo *c = &infos[char_id];
			
			g_string_append(body, vert_line);
			
			if (img[j] != NULL)
			{
				uint16_t unicode;
				char *debuginfo = NULL;
				char *src;
				
				unicode = c->char_index < BICS_COUNT ? Bics2Unicode[c->char_index] : UNDEFINED;
				if (debug)
				{
					debuginfo = g_strdup_printf("Width: %u Height %u&#10;Ascent: %d Descent: %d lb: %d rb: %d&#10;",
						c->bbox.width, c->bbox.height, c->bbox.ascent, c->bbox.descent, c->bbox.lbearing, c->bbox.rbearing);
				}
				if (specs.output_mode == MODE_OUTLINE)
					src = g_strdup_printf("<svg width=\"%d\" height=\"%d\"><use xlink:href=\"%s#chr%04x\" style=\"text-align: left; vertical-align: top;\"></use></svg>",
						font_bb.width, font_bb.height,
						img[j], char_id);
				else
					src = g_strdup_printf("<img alt=\"\" style=\"text-align: left; vertical-align: top; position: relative; left: %dpx; top: %dpx\" src=\"%s\">",
						-(font_bb.lbearing - c->bbox.lbearing),
						font_bb.ascent - c->bbox.ascent,
						img[j]);
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx; min-width: %dpx; min-height: %dpx;\" title=\""
					"Index: 0x%x (%u)&#10;"
					"ID: 0x%04x&#10;"
					"Unicode: 0x%04x &#%u;&#10;"
					"%s&#10;"
					"Xmin: %7.2f Ymin: %7.2f&#10;"
					"Xmax: %7.2f Ymax: %7.2f&#10;%s"
					"\">%s</td>",
					font_bb.width, font_bb.height,
					font_bb.width, font_bb.height,
					c->char_index, c->char_index,
					c->char_id,
					unicode, unicode,
					ucd_get_name(unicode),
					(double)c->bbox.xmin / 65536.0,
					(double)c->bbox.ymin / 65536.0,
					(double)c->bbox.xmax / 65536.0,
					(double)c->bbox.ymax / 65536.0,
					debuginfo ? debuginfo : "",
					src);
				g_free(debuginfo);
				g_free(src);
			} else
			{
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx;\"></td>", 0, 0);
			}
		}				
			
		g_string_append(body, vert_line);
		g_string_append(body, "</tr>\n");
		gen_hor_line(body, columns);
		if (optimize_output && !any_defined_line)
		{
			g_string_truncate(body, line_start);
		}	
	}
	if ((id % PAGE_SIZE) != 0)
		gen_hor_line(body, columns);
	if (optimize_output && !any_defined_page)
		g_string_truncate(body, page_start);
	g_string_append(body, "</table>\n");

	for (i = 0; i < num_ids; i++)
	{
		g_free(infos[i].local_filename);
		g_free(infos[i].url);
	}
	g_free(infos);
	infos = NULL;
	g_free(basename);
		
	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static gboolean load_speedo_font(const char *filename, GString *body)
{
	gboolean ret;
	ufix8 tmp[FH_FBFSZ + 4];
	ufix32 minbufsize;
	gboolean got_header = FALSE;
	
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		g_string_append_printf(errorout, "%s: %s\n", filename, strerror(errno));
		ret = FALSE;
	} else
	{
		if (fread(tmp, sizeof(tmp), 1, fp) != 1)
		{
			g_string_append_printf(errorout, "%s: read error\n", filename);
			ret = FALSE;
		} else if (read_4b(tmp + FH_FMVER + 4) != 0x0d0a0000L)
		{
			sp_report_error(4);
		} else
		{
			g_free(font_buffer);
			g_free(c_buffer);
			c_buffer = NULL;
			minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
			font_buffer = g_new(ufix8, minbufsize);
			if (font_buffer == NULL)
			{
				g_string_append_printf(errorout, "%s\n", strerror(errno));
				ret = FALSE;
			} else
			{
				fseek(fp, 0, SEEK_SET);
				if (fread(font_buffer, minbufsize, 1, fp) != 1)
				{
					g_free(font_buffer);
					font_buffer = NULL;
					g_string_append_printf(errorout, "%s: read error\n", filename);
					ret = FALSE;
				} else
				{
					mincharsize = read_2b(font_buffer + FH_CBFSZ);
				
					c_buffer = g_new(ufix8, mincharsize);
					if (c_buffer == NULL)
					{
						g_free(font_buffer);
						font_buffer = NULL;
						g_string_append_printf(errorout, "%s\n", strerror(errno));
						ret = FALSE;
					} else
					{
						font.org = font_buffer;
						font.no_bytes = minbufsize;
					
						got_header = TRUE;
						ret = gen_speedo_font(body);
					
						g_free(font_buffer);
						font_buffer = NULL;
						g_free(c_buffer);
						c_buffer = NULL;
					}
				}
			}
		}
		fclose(fp);
	}
	
	if (!got_header)
		html_out_header(body, NULL, NULL, xbasename(filename), FALSE);
	html_out_trailer(body, FALSE);

	return ret;
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
	char *val;
	
	errorfile = fopen("error.log", "a");
	if (errorfile == NULL)
		errorfile = stderr;
	errorout = g_string_new(NULL);
	
	body = g_string_new(NULL);
	cgiInit(body);

	if (cgiScriptFilename == NULL || cgiRemoteAddr == NULL)
		return 1;

	{
		char *dir = spd_path_get_dirname(cgiScriptFilename);
		char *cache_dir = g_build_filename(dir, cgi_cachedir, NULL);
		DIR *dp;
		struct dirent *e;
		
		output_dir = g_build_filename(cache_dir, cgiRemoteAddr, NULL);
		output_url = g_build_filename(cgi_cachedir, cgiRemoteAddr, NULL);

		if (mkdir(cache_dir, 0750) < 0 && errno != EEXIST)
			g_string_append_printf(errorout, "%s: %s\n", cache_dir, strerror(errno));
		if (mkdir(output_dir, 0750) < 0 && errno != EEXIST)
			g_string_append_printf(errorout, "%s: %s\n", output_dir, strerror(errno));
		
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
				if ((p = strrchr(e->d_name, '.')) != NULL &&
					(strcmp(p, ".png") == 0 || strcmp(p, ".svg") == 0))
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
	
	point_size = 120;
	if ((val = cgiFormString("points")) != NULL)
	{
		point_size = strtol(val, NULL, 10);
		g_free(val);
		if (point_size < 40 || point_size > 10000)
			point_size = 120;
	}
	
	quality = 1;
	if ((val = cgiFormString("quality")) != NULL)
	{
		quality = (int)strtol(val, NULL, 10);
		g_free(val);
	}
	
	cgi_cached = FALSE;
	if ((val = cgiFormString("cached")) != NULL)
	{
		cgi_cached = (int)strtol(val, NULL, 10) != 0;
		g_free(val);
	}

	x_res = y_res = 72;
	if ((val = cgiFormString("resolution")) != NULL)
	{
		x_res = y_res = (int)strtol(val, NULL, 10);
		if (x_res < 10 || x_res > 10000)
			x_res = y_res = 72;
		g_free(val);
	}
	if ((val = cgiFormString("xresolution")) != NULL)
	{
		x_res = (int)strtol(val, NULL, 10);
		if (x_res < 10 || x_res > 10000)
			x_res = 72;
		g_free(val);
	}
	if ((val = cgiFormString("yresolution")) != NULL)
	{
		y_res = (int)strtol(val, NULL, 10);
		if (y_res < 10 || y_res > 10000)
			y_res = 72;
		g_free(val);
	}
	
	hidemenu = FALSE;
	if ((val = cgiFormString("hidemenu")) != NULL)
	{
		hidemenu = strtol(val, NULL, 10) != 0;
		g_free(val);
	}
	
	debug = FALSE;
	if ((val = cgiFormString("debug")) != NULL)
	{
		debug = strtol(val, NULL, 10) != 0;
		g_free(val);
	}
	
	if (g_ascii_strcasecmp(cgiRequestMethod, "GET") == 0)
	{
		char *url = cgiFormString("url");
		char *filename = g_strdup(url);
		char *scheme = empty(filename) ? g_strdup("undefined") : uri_has_scheme(filename) ? g_strndup(filename, strchr(filename, ':') - filename) : g_strdup("file");
		
		if (filename && filename[0] == '/')
		{
			html_referer_url = filename;
			filename = g_strconcat(cgiDocumentRoot, filename, NULL);
		} else if (empty(xbasename(filename)) || (!cgi_cached && g_ascii_strcasecmp(scheme, "file") == 0))
		{
			/*
			 * disallow file URIs, they would resolve to local files on the WEB server
			 */
			html_out_header(body, NULL, NULL, _("403 Forbidden"), TRUE);
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
			if (!cgi_cached &&
				(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK ||
				(curl = curl_easy_init()) == NULL))
			{
				html_out_header(body, NULL, NULL, _("500 Internal Server Error"), TRUE);
				g_string_append(body, _("could not initialize curl\n"));
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				char *local_filename;
				
				if (cgi_cached)
				{
					html_referer_url = g_strdup(xbasename(filename));
					local_filename = g_build_filename(output_dir, html_referer_url, NULL);
					g_free(filename);
					filename = local_filename;
				} else
				{
					html_referer_url = g_strdup(filename);
					local_filename = curl_download(curl, body, filename);
					g_free(filename);
					filename = local_filename;
					if (filename)
						cgi_cached = TRUE;
				}
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
			html_out_header(body, NULL, NULL, _("403 Forbidden"), TRUE);
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
				html_out_header(body, NULL, NULL, _("404 Not Found"), TRUE);
				g_string_append_printf(body, "%s: %s\n", xbasename(filename), err);
				html_out_trailer(body, TRUE);
				retval = EXIT_FAILURE;
			} else
			{
				data = cgiFormFileData("file", &len);
				fwrite(data, 1, len, fp);
				fclose(fp);
				cgi_cached = TRUE;
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
	g_string_free(errorout, TRUE);
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	
	fflush(errorfile);
	if (errorfile != stderr)
		fclose(errorfile);
	
	return retval;
}
