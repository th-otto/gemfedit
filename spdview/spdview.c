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
#include "vdimaps.h"
#include "bics2uni.h"

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
static GString *errorout;
static FILE *errorfile;
static size_t body_start;
static gboolean cgi_cached;

#define _(x) x

#define DEBUG 0

static int force_crlf = FALSE;

#define	MAX_BITS	1024
#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

static unsigned char framebuffer[MAX_BITS][MAX_BITS];
static uint16_t char_index, char_id;
static buff_t font;
static buff_t char_data;
static ufix8 *font_buffer;
static ufix8 *c_buffer;
static uint16_t mincharsize;
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
	bbox_t bbox;
} charinfo;
static charinfo *infos;

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

#if 0
static char *html_cgi_params(void)
{
	return g_strdup_printf("%s&amp;quality=%d&amp;points=%ld",
		cgi_cached ? "&amp;cached=1" : "",
		quality,
		point_size);
}
#endif
	
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

	body_start = out->len;
	
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
	
	if (errorout->len)
	{
		g_string_insert_len(errorout, 0, 
			"<div class=\"spdview_error_note\">\n"
			"<pre>\n",
			-1);
		g_string_append(errorout,
			"</pre>\n"
			"</div>\n");
		g_string_insert_len(out, body_start, errorout->str, errorout->len);
		g_string_truncate(errorout, 0);
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
	g_string_append_vprintf(errorout, str, v);
	g_string_append(errorout, "\n");
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
		g_string_append_printf(errorout, "char 0x%x (0x%x): wider than max bits (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
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
		g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & width mismatch (%ld vs %d)\n",
				char_index, char_id, (unsigned long)(bb.xmax - bb.xmin) >> 16, bit_width);
	if (((bb.ymax - bb.ymin) >> 16) != bit_height)
		g_string_append_printf(errorout, "char 0x%x (0x%x): bbox & height mismatch (%ld vs %d)\n",
				char_index, char_id, (unsigned long)(bb.ymax - bb.ymin) >> 16, bit_height);
	if ((bb.xmin >> 16) != off_horz)
		g_string_append_printf(errorout, "char 0x%x (0x%x): x min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.xmin >> 16, off_horz);
	if ((bb.ymin >> 16) != off_vert)
		g_string_append_printf(errorout, "char 0x%x (0x%x): y min mismatch (%ld vs %d)\n", char_index, char_id, (unsigned long)bb.ymin >> 16, off_vert);
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
		g_string_append_printf(errorout, "char 0x%x (0x%x): width too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
		bit_width = MAX_BITS;
	}
	
	if (bit_height > MAX_BITS)
	{
		g_string_append_printf(errorout, "char 0x%x (0x%x): height too large (%d vs %d)\n", char_index, char_id, bit_width, MAX_BITS);
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
		g_string_append_printf(errorout, "char 0x%x (0x%x): bit1 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit1, bit_width);
		xbit1 = MAX_BITS;
		trunc = 1;
	}
	if (xbit2 < 0 || xbit2 > bit_width)
	{
		g_string_append_printf(errorout, "char 0x%x (0x%x): bit2 %d wider than max bits %u -- truncated\n", char_index, char_id, xbit2, bit_width);
		xbit2 = MAX_BITS;
		trunc = 1;
	}

	if (y < 0 || y >= bit_height)
	{
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

buff_t *sp_load_char_data(fix31 file_offset, fix15 num, fix15 cb_offset)
{
	if (fseek(fp, (long) file_offset, (int) 0))
	{
		g_string_append_printf(errorout, "can't seek to char\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if ((num + cb_offset) > mincharsize)
	{
		g_string_append_printf(errorout, "char buf overflow\n");
		char_data.org = c_buffer;
		char_data.no_bytes = 0;
		return &char_data;
	}
	if (fread((c_buffer + cb_offset), sizeof(ufix8), num, fp) != num)
	{
		g_string_append_printf(errorout, "can't get char data\n");
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
	const charinfo *cinfo = &infos[char_id];
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
	for (i = 0; i < (columns * 2 + 1); i++)
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
		specs.flags = MODE_BLACK;
		break;
	case 1:
	default:
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
	make_font_filename(fontfilename, fontname);
	
	if (!sp_set_specs(&specs))
	{
		decode_ok = FALSE;
	} else
	{
		time_t t;
		struct tm tm;
		char *basename;
		int columns = 0;
		
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
			if (char_id != 0 && char_id != 0xffff && char_id >= num_ids)
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
			infos[i].char_id = 0xffff;
			infos[i].local_filename = NULL;
			infos[i].url = NULL;
		}
		for (i = 0; i < num_chars; i++)
		{
			char_index = i + first_char_index;
			char_id = sp_get_char_id(char_index);
			if (char_id != 0 && char_id != 0xffff)
			{
				if (infos[char_id].char_id != 0xffff)
				{
					g_string_append_printf(errorout, "char 0x%x (0x%x) already defined\n", char_index, char_id);
				} else
				{
					infos[char_id].char_index = char_index;
					infos[char_id].char_id = char_id;
					infos[char_id].local_filename = g_strdup_printf("%s/%schr%04x.png", output_dir, basename, char_id);
					infos[char_id].url = g_strdup_printf("%s/%schr%04x.png", output_url, basename, char_id);
					if (!sp_make_char(char_index))
					{
						g_string_append_printf(errorout, "can't make char 0x%x (0x%x)\n", char_index, char_id);
					}
				}
			}
		}
		
#if 0
		qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
#endif
		
		g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
		for (id = 0; id < num_ids; id += CHAR_COLUMNS)
		{
			gboolean defined[CHAR_COLUMNS];
			const char *klass;
			const char *img[CHAR_COLUMNS];
			static char const vert_line[] = "<td class=\"vertical_line\"></td>\n";
			
			if (id != 0 && (id % PAGE_SIZE) == 0)
				g_string_append(body, "<tr><td width=\"1\" height=\"10\" style=\"padding: 0px; margin:0px;\"></td></tr>\n");

			columns = CHAR_COLUMNS;
			if ((id + columns) > num_ids)
				columns = num_ids - id;
			
			if ((id % PAGE_SIZE) == 0)
				gen_hor_line(body, columns);
			
			g_string_append(body, "<tr>\n");
			for (j = 0; j < columns; j++)
			{
				uint16_t char_id = id + j;
				defined[j] = FALSE;
				img[j] = NULL;
				if (infos[char_id].char_id != 0xffff)
				{
					defined[j] = TRUE;
					if (infos[char_id].url != NULL)
						img[j] = infos[char_id].url;
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
				g_string_append(body, vert_line);
				
				if (img[j] != NULL)
				{
					uint16_t unicode;
					
					unicode = infos[char_id].char_index < BICS_COUNT ? Bics2Unicode[infos[char_id].char_index] : 0xffff;
					g_string_append_printf(body,
						"<td class=\"spd_glyph_image\" title=\""
						"Index: 0x%x&#10;"
						"ID: 0x%04x&#10;"
						"Unicode: 0x%04x&#10;"
						"Xmin: %7.2f Ymin: %7.2f&#10;"
						"Xmax: %7.2f Ymax: %7.2f&#10;"
						"\"><img alt=\"\" src=\"%s\"></td>",
						infos[char_id].char_index,
						infos[char_id].char_id,
						unicode,
						(double)infos[char_id].bbox.xmin / 65536.0,
						(double)infos[char_id].bbox.ymin / 65536.0,
						(double)infos[char_id].bbox.xmax / 65536.0,
						(double)infos[char_id].bbox.ymax / 65536.0,
						img[j]);
				} else
				{
					g_string_append_printf(body,
						"<td class=\"spd_glyph_image\" style=\"width: 0px; height: 0px;\"></td>");
				}
			}				
				
			g_string_append(body, vert_line);
			g_string_append(body, "</tr>\n");
			gen_hor_line(body, columns);
		}
		if ((id % PAGE_SIZE) != 0)
			gen_hor_line(body, columns);
		g_string_append(body, "</table>\n");

		for (i = 0; i < num_ids; i++)
		{
			g_free(infos[i].local_filename);
			g_free(infos[i].url);
		}
		g_free(infos);
		infos = NULL;
		g_free(basename);
	}
		
	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static gboolean load_speedo_font(const char *filename, GString *body)
{
	gboolean ret;
	ufix8 tmp[16];
	ufix32 minbufsize;
	
	html_out_header(body, xbasename(filename), FALSE);

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		g_string_append_printf(errorout, "%s: %s\n", filename, strerror(errno));
		ret = FALSE;
	} else
	{
		if (fread(tmp, sizeof(ufix8), 16, fp) != 16)
		{
			fclose(fp);
			g_string_append_printf(errorout, "%s: read error\n", filename);
			ret = FALSE;
		} else
		{
			g_free(font_buffer);
			g_free(c_buffer);
			c_buffer = NULL;
			minbufsize = (ufix32) read_4b(tmp + FH_FBFSZ);
			font_buffer = g_new(ufix8, minbufsize);
			if (font_buffer == NULL)
			{
				fclose(fp);
				g_string_append_printf(errorout, "%s\n", strerror(errno));
				ret = FALSE;
			} else
			{
				fseek(fp, 0, SEEK_SET);
				if (fread(font_buffer, sizeof(ufix8), minbufsize, fp) != minbufsize)
				{
					fclose(fp);
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
						fclose(fp);
						g_free(font_buffer);
						font_buffer = NULL;
						g_string_append_printf(errorout, "%s\n", strerror(errno));
						ret = FALSE;
					} else
					{
						font.org = font_buffer;
						font.no_bytes = minbufsize;
					
						ret = gen_speedo_font(filename, body);
						fclose(fp);
					
						g_free(font_buffer);
						font_buffer = NULL;
						g_free(c_buffer);
						c_buffer = NULL;
					}
				}
			}
		}
	}
		
	html_out_trailer(body, FALSE);

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
			g_string_append_printf(errorout, "%s: %s\n", parms->filename, strerror(errno));
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
	char *val;
	
	errorfile = fopen("error.log", "a");
	if (errorfile == NULL)
		errorfile = stderr;
	errorout = g_string_new(NULL);
	
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
		g_free(val);
	}
	if ((val = cgiFormString("xresolution")) != NULL)
	{
		x_res = (int)strtol(val, NULL, 10);
		g_free(val);
	}
	if ((val = cgiFormString("yresolution")) != NULL)
	{
		y_res = (int)strtol(val, NULL, 10);
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
			if (!cgi_cached &&
				(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK ||
				(curl = curl_easy_init()) == NULL))
			{
				html_out_header(body, _("500 Internal Server Error"), TRUE);
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
