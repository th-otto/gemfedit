#include "linux/libcwrap.h"
#include "defs.h"
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <dirent.h>
#include "writepng.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftbitmap.h>
#include "cgic.h"
#include "vdimaps.h"
#include "cgiutil.h"
#include "ucd.h"
#include "version.h"

char const gl_program_name[] = "ttfview.cgi";
const char *cgi_scriptname = "ttfview.cgi";

typedef struct bbox_tag {
	int32_t   xmin;
	int32_t   xmax;
	int32_t   ymin;
	int32_t   ymax;
} bbox_t;

typedef struct _glyphinfo_t {
	FT_Short ascent;
	FT_Short descent;
	FT_Short lbearing;
	FT_Short rbearing;
	FT_Short off_vert;
	FT_UShort width;
	FT_UShort height;
	FT_Pos xmin;
	FT_Pos ymin;
	FT_Pos xmax;
	FT_Pos ymax;
} glyphinfo_t;

#define DEBUG 0

#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

static FT_Library ft_library = NULL;
static FT_Face face = NULL;
static FT_Long face_index = 0;
static uint32_t num_ids;
static glyphinfo_t font_bb;
static char fontname[70 + 1];
static char fontfilename[70 + 1];

static long point_size = 120;
static int x_res = 72;
static int y_res = 72;
static int quality = 1;
static gboolean optimize_output = TRUE;

#define HAVE_MKSTEMPS

typedef struct {
	uint16_t char_index;
	uint32_t char_id;
	char *local_filename;
	char *url;
	glyphinfo_t bbox;
	char name[32];
} charinfo;
static charinfo *infos;

#define UNDEFINED 0

static int output_mode;
#define MODE_OUTLINE 1

static FT_Int32 load_flags = FT_LOAD_DEFAULT;
static FT_Render_Mode render_mode = FT_RENDER_MODE_LCD;

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

static GString *get_font_info(FT_Face font)
{
	GString *out = g_string_new(NULL);
	(void) font;
	return out;
}

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

static gboolean ft_get_char_bbox(uint16_t char_index, bbox_t *bb)
{
	FT_Glyph glyph;
	FT_GlyphSlot slot;
	
	if (FT_Load_Glyph(face, char_index, load_flags | FT_LOAD_NO_BITMAP) != FT_Err_Ok)
		return FALSE;
	slot = face->glyph;
	FT_Get_Glyph(slot, &glyph);
	bb->xmin = -slot->metrics.horiBearingX;
	bb->ymin = -slot->metrics.horiBearingY;
	bb->xmax = slot->metrics.width;
	bb->ymax = slot->metrics.height;
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static void update_bbox(charinfo *c, glyphinfo_t *box)
{
	bbox_t bb;
	
	if (!ft_get_char_bbox(c->char_index, &bb))
		return;
	c->bbox.xmin = bb.xmin;
	c->bbox.ymin = bb.ymin;
	c->bbox.xmax = bb.xmax;
	c->bbox.ymax = bb.ymax;
	box->xmin = MIN(box->xmin, c->bbox.xmin);
	box->ymin = MIN(box->ymin, c->bbox.ymin);
	box->xmax = MIN(box->xmax, c->bbox.xmax);
	box->ymax = MIN(box->ymax, c->bbox.ymax);
	c->bbox.width = ((bb.xmax - bb.xmin) + 32L) >> 6;
	c->bbox.height = ((bb.ymax - bb.ymin) + 32L) >> 6;
	c->bbox.lbearing = (bb.xmin + 32L) >> 6;
	c->bbox.off_vert = (bb.ymin - (bb.ymax - bb.ymin) + (bb.ymax - bb.ymin)) >> 6;
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

static gboolean write_png(const charinfo *c, FT_Bitmap *source)
{
	int i;
	writepng_info *info;
	int rc;
	FT_Bitmap target = { 0, 0, 0, 0, 0, FT_PIXEL_MODE_NONE, 0, 0 };
	
	info = writepng_new();
	/*
	 * we cannot write a 0x0 image :(
	 */
	if (info->width == 0)
		info->width = 1;
	if (info->height == 0)
		info->height = 1;

	info->bpp = 8;
	info->width = source->width;
	info->height = source->rows;

	switch (source->pixel_mode)
	{
	case FT_PIXEL_MODE_MONO:
		info->num_palette = 2;
		info->have_bg = vdi_maptab256[0];
		break;

	case FT_PIXEL_MODE_GRAY:
		info->num_palette = source->num_grays;
		break;

	case FT_PIXEL_MODE_GRAY2:
	case FT_PIXEL_MODE_GRAY4:
		FT_Bitmap_Convert(ft_library, source, &target, 1);
		source = &target;
		info->num_palette = source->num_grays;
		break;

	case FT_PIXEL_MODE_LCD:
		info->bpp = 24;
		info->width /= 3;
		info->num_palette = 0;
		break;

	case FT_PIXEL_MODE_LCD_V:
		info->bpp = 24;
		info->height /= 3;
		info->num_palette = 0;
		break;

	case FT_PIXEL_MODE_BGRA:
		info->bpp = 32;
		info->num_palette = 0;
		info->swapped = TRUE;
		break;

	default:
		return FALSE;
	}

	info->rowbytes = source->pitch;

	info->image_data = source->buffer;
	info->x_res = (x_res * 10000L) / 254;
	info->y_res = (y_res * 10000L) / 254;

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
	
	if (target.pixel_mode != FT_PIXEL_MODE_NONE)
		FT_Bitmap_Done(ft_library, &target);
	
	return rc == 0;
}

/* ------------------------------------------------------------------------- */

static gboolean ft_make_char(charinfo *c)
{
	FT_Glyph glyf;
	FT_GlyphSlot slot;
	FT_BitmapGlyph bitmap;
	FT_Bitmap *source;
	
	if (FT_Load_Glyph(face, c->char_index, load_flags) != FT_Err_Ok)
		return FALSE;
	slot = face->glyph;
	FT_Get_Glyph(slot, &glyf);
	if (glyf->format != FT_GLYPH_FORMAT_BITMAP)
	{
		if (FT_Glyph_To_Bitmap(&glyf, render_mode, NULL, 0) != FT_Err_Ok)
			return FALSE;
	}
	if (glyf->format != FT_GLYPH_FORMAT_BITMAP)
	{
		g_string_append(errorout, "invalid glyph format returned!\n");
		return FALSE;
	}

	bitmap = (FT_BitmapGlyph) glyf;
	source = &bitmap->bitmap;

	write_png(c, source);
	
	FT_Done_Glyph(glyf);
	
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static gboolean gen_ttf_font(GString *body)
{
	gboolean decode_ok = TRUE;
	time_t t;
	struct tm tm;
	char *basename;
	int columns;
	FT_UInt gindex;
	uint32_t char_id, id;
	uint16_t j;
	charinfo *c;
	GString *font_info;
	size_t page_start;
	size_t line_start;
	gboolean any_defined_page, any_defined_line;
	
	make_font_filename(fontfilename, fontname);
	font_info = get_font_info(face);
	html_out_header(body, font_info, fontname, FALSE);
	g_string_free(font_info, TRUE);
	
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
	char_id = FT_Get_First_Char(face, &gindex);
	while (gindex != 0)
	{
		if (char_id != UNDEFINED && char_id >= num_ids)
			num_ids = char_id + 1;
		char_id = FT_Get_Next_Char(face, char_id, &gindex);
		/*
				if (FT_HAS_GLYPH_NAMES(face))
					FT_Get_Glyph_Name(face, gindex, buf, 32);
				else
					buf[0] = '\0';
		*/
	}
		
	if (num_ids == 0)
	{
		g_string_append(errorout, "no characters in font\n");
		return FALSE;
	} 

	infos = g_new(charinfo, num_ids);
	if (infos == NULL)
	{
		g_string_append_printf(errorout, "%s\n", strerror(errno));
		return FALSE;
	}
		
	for (char_id = 0; char_id < num_ids; char_id++)
	{
		infos[char_id].char_index = 0;
		infos[char_id].char_id = UNDEFINED;
		infos[char_id].local_filename = NULL;
		infos[char_id].url = NULL;
		infos[char_id].name[0] = '\0';
	}

	/*
	 * determine the average width of all the glyphs in the font
	 */
	{
		unsigned long total_width;
		uint16_t num_glyphs;
		glyphinfo_t max_bb;
		FT_Short max_lbearing;
		FT_Short min_descent;
		FT_Short average_width;
		
		total_width = 0;
		num_glyphs = 0;
		max_bb.width = 0;
		max_bb.height = 0;
		max_bb.off_vert = 0;
		max_bb.ascent = -32000;
		max_bb.descent = -32000;
		max_bb.rbearing = -32000;
		max_bb.lbearing = 32000;
		max_bb.xmin = 32000L << 6;
		max_bb.ymin = 32000L << 6;
		max_bb.xmax = -(32000L << 6);
		max_bb.ymax = -(32000L << 6);
		max_lbearing = -32000;
		min_descent = 32000;
		for (char_id = 0; char_id < num_ids; char_id++)
		{
			c = &infos[char_id];
			
			gindex = FT_Get_Char_Index(face, char_id);
			if (gindex != 0)
			{
				if (c->char_id != UNDEFINED)
				{
					if (debug)
						g_string_append_printf(errorout, "char 0x%x (0x%lx) already defined\n", gindex, (unsigned long)char_id);
				} else
				{
					c->char_index = gindex;
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
			average_width = (FT_Short)(total_width / num_glyphs);
		if (debug)
		{
			g_string_append_printf(errorout, "max height %d max width %d average width %d\n", max_bb.height, max_bb.width, average_width);
		}
		
		max_bb.height = max_bb.ascent + max_bb.descent;
		max_bb.width = max_bb.rbearing - max_bb.lbearing;

		if (debug)
		{
			FT_Pos xmin, ymin, xmax, ymax;
			long fwidth, fheight;
			long pixel_size = (point_size * x_res + 360) / 720;
			
			xmin = face->bbox.xMin;
			ymin = face->bbox.yMin;
			xmax = face->bbox.xMax;
			ymax = face->bbox.yMax;
			fwidth = xmax - xmin;
			fwidth = fwidth * pixel_size / face->units_per_EM;
			fheight = ymax - ymin;
			fheight = fheight * pixel_size / face->units_per_EM;
			g_string_append_printf(errorout, "bbox: %d %d ascent %d descent %d lb %d rb %d\n",
				max_bb.width, max_bb.height,
				max_bb.ascent, max_bb.descent,
				max_bb.lbearing, max_bb.rbearing);
			g_string_append_printf(errorout, "bbox (header): %ld %ld %ld %ld; %ld %ld %ld %ld\n",
				xmin, ymin,
				xmax, ymax,
				xmin * pixel_size / face->units_per_EM,
				ymin * pixel_size / face->units_per_EM,
				xmax * pixel_size / face->units_per_EM,
				ymax * pixel_size / face->units_per_EM);
		}
		
		font_bb = max_bb;
	}
	
	for (char_id = 0; char_id < num_ids; char_id++)
	{
		c = &infos[char_id];
		if (c->char_id != UNDEFINED)
		{
			const char *ext = output_mode == MODE_OUTLINE ? ".svg" : ".png";
			c->local_filename = g_strdup_printf("%s/%schr%04x%s", output_dir, basename, char_id, ext);
			c->url = g_strdup_printf("%s/%schr%04x%s", output_url, basename, char_id, ext);
			if (!ft_make_char(c))
			{
				g_string_append_printf(errorout, "can't make char 0x%x (0x%lx)\n", c->char_index, (unsigned long)char_id);
			}
		}
	}
	
#if 0
	qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
#endif
		
	g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
	page_start = body->len;
	any_defined_page = FALSE;
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
			char_id = id + j;
			c = &infos[char_id];
			
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
			char_id = id + j;
			c = &infos[char_id];
			
			g_string_append(body, vert_line);
			
			if (img[j] != NULL)
			{
				uint32_t unicode;
				char *debuginfo = NULL;
				char *src;
				
				unicode = char_id;
				if (debug)
				{
					debuginfo = g_strdup_printf("Width: %u Height %u&#10;Ascent: %d Descent: %d lb: %d rb: %d&#10;",
						c->bbox.width, c->bbox.height, c->bbox.ascent, c->bbox.descent, c->bbox.lbearing, c->bbox.rbearing);
				}
				if (output_mode == MODE_OUTLINE)
					src = g_strdup_printf("<svg width=\"%d\" height=\"%d\"><use xlink:href=\"%s#chr%04lx\" style=\"text-align: left; vertical-align: top;\"></use></svg>",
						font_bb.width, font_bb.height,
						img[j], (unsigned long)char_id);
				else
					src = g_strdup_printf("<img alt=\"\" style=\"text-align: left; vertical-align: top; position: relative; left: %dpx; top: %dpx\" src=\"%s\">",
						-(font_bb.lbearing - c->bbox.lbearing),
						font_bb.ascent - c->bbox.ascent,
						img[j]);
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx; min-width: %dpx; min-height: %dpx;\" title=\""
					"Index: 0x%x (%u)&#10;"
					"ID: 0x%04lx&#10;"
					"Unicode: 0x%04lx &#%lu;&#10;"
					"%s&#10;"
					"Xmin: %7.2f Ymin: %7.2f&#10;"
					"Xmax: %7.2f Ymax: %7.2f&#10;%s"
					"\">%s</td>",
					font_bb.width, font_bb.height,
					font_bb.width, font_bb.height,
					c->char_index, c->char_index,
					(unsigned long)c->char_id,
					(unsigned long)unicode, (unsigned long)unicode,
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

	for (char_id = 0; char_id < num_ids; char_id++)
	{
		g_free(infos[char_id].local_filename);
		g_free(infos[char_id].url);
	}
	g_free(infos);
	infos = NULL;
	g_free(basename);

	return decode_ok;
}

/* ------------------------------------------------------------------------- */

static gboolean load_ttf_font(const char *filename, GString *body)
{
	gboolean ret;
	FT_Error ft_error;
	gboolean got_header = FALSE;
	
	ft_error = FT_New_Face(ft_library, filename, face_index, &face);
	if (ft_error == FT_Err_Ok)
		ft_error = FT_Set_Char_Size(face, (point_size << 6) / 10, 0, x_res, y_res);
	if (ft_error == FT_Err_Ok)
		ft_error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if (ft_error != FT_Err_Ok)
	{
		g_string_append_printf(errorout, "%s: %s\n", filename, FT_Strerror(ft_error));
		ret = FALSE;
	} else
	{
		got_header = TRUE;
		chomp(fontname, face->family_name ? face->family_name : xbasename(filename), sizeof(fontname));
		ret = gen_ttf_font(body);
	}

	if (!got_header)
		html_out_header(body, NULL, xbasename(filename), FALSE);
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
	FT_Error ft_error;
	
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
	
	face_index = 0;
	if ((val = cgiFormString("index")) != NULL)
	{
		face_index = strtol(val, NULL, 10);
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
	
	ft_error = FT_Init_FreeType(&ft_library);
	if (ft_error)
	{
		html_out_header(body, NULL, _("500 Internal Server Error"), TRUE);
		g_string_append(body, _("could not initialize freetype\n"));
		html_out_trailer(body, TRUE);
		retval = EXIT_FAILURE;
	}
	
	if (retval == EXIT_SUCCESS && g_ascii_strcasecmp(cgiRequestMethod, "GET") == 0)
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
			html_out_header(body, NULL, _("403 Forbidden"), TRUE);
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
				html_out_header(body, NULL, _("500 Internal Server Error"), TRUE);
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
			if (load_ttf_font(filename, body) == FALSE)
			{
				retval = EXIT_FAILURE;
			}
			g_free(filename);
		}
		g_free(scheme);
		g_free(html_referer_url);
		html_referer_url = NULL;
		
		g_free(url);
	} else if (retval == EXIT_SUCCESS &&
		(g_ascii_strcasecmp(cgiRequestMethod, "POST") == 0 ||
		 g_ascii_strcasecmp(cgiRequestMethod, "BOTH") == 0))
	{
		char *filename;
		int len;
		
		g_string_truncate(body, 0);
		filename = cgiFormFileName("file", &len);
		if (filename == NULL || len == 0)
		{
			const char *scheme = "undefined";
			html_out_header(body, NULL, _("403 Forbidden"), TRUE);
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
				html_out_header(body, NULL, _("404 Not Found"), TRUE);
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
				if (load_ttf_font(local_filename, body) == FALSE)
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
	
	if (ft_library)
		FT_Done_FreeType(ft_library);
	
	if (curl)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	
	fflush(errorfile);
	if (errorfile != stderr)
		fclose(errorfile);

	(void) fp;
	
	return retval;
}
