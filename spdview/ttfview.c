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
#include "cgiutil.h"
#include "ucd.h"
#include "version.h"

/*
 * for 8bpp gray bitmap, use a 16x16 area
 */
#define BPP8_MUL 16

char const gl_program_name[] = "ttfview.cgi";
const char *cgi_scriptname = "ttfview.cgi";
char const html_nav_load_href[] = "ttfview.php";
static char const glyph_bg_class[] = "ttfview_glyph_bg";

typedef struct bbox_tag {
	int32_t   xmin;
	int32_t   xmax;
	int32_t   ymin;
	int32_t   ymax;
} bbox_t;

typedef struct _glyphinfo_t {
	FT_Int ascent;
	FT_Int descent;
	FT_Int lbearing;
	FT_Int rbearing;
	FT_UInt width;
	FT_UInt height;
	FT_Int advance;
} glyphinfo_t;

#define DEBUG 0

#define CHAR_COLUMNS 16
#define PAGE_SIZE    128

#define PNG_GLYPHS_PER_ROW 128

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
static int bitsPerPixel;
static int bpp_mul;
static size_t big_bytes_per_row;
static size_t big_bytes_per_glyph_row;
static uint8_t *big_buffer;
static size_t png_glyph_rows;


#define HAVE_MKSTEMPS

typedef struct {
	uint16_t char_index;
	uint32_t char_id;
	const char *basename;
	glyphinfo_t bbox;
	char name[32];
} charinfo;
static charinfo *infos;

#define UNDEFINED 0

#define MODE_OUTLINE 1

static FT_Int32 load_flags = FT_LOAD_DEFAULT;
static FT_Render_Mode render_mode = FT_RENDER_MODE_MONO;

/*****************************************************************************/
/* ------------------------------------------------------------------------- */
/*****************************************************************************/

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

static gboolean write_big_png(const char *basename)
{
	int i;
	writepng_info *info;
	int rc;
	char *local_filename;
	
	info = writepng_new();
	/*
	 * we cannot write a 0x0 image :(
	 */
	if (info->width == 0)
		info->width = 1;
	if (info->height == 0)
		info->height = 1;

	info->bpp = 8;
	info->width = font_bb.width * PNG_GLYPHS_PER_ROW;
	info->height = font_bb.height * png_glyph_rows;

	if (bitsPerPixel == 1)
	{
		info->num_palette = 2;
		info->have_bg = 0;
	} else
	{
		info->num_palette = 256;
	}
	if (debug)
	{
		g_string_append_printf(errorout, "writing <a href=\"%s/%s.png\">%s.png</a> %ldx%ld\n", output_url, basename, basename, info->width, info->height);
	}
	info->rowbytes = big_bytes_per_row * PNG_GLYPHS_PER_ROW;

	info->image_data = big_buffer;
	info->x_res = (x_res * 10000L) / 254;
	info->y_res = (y_res * 10000L) / 254;

	local_filename = g_strdup_printf("%s/%s.png", output_dir, basename);
	
	info->outfile = fopen(local_filename, "wb");
	if (info->outfile == NULL)
	{
		rc = errno;
	} else
	{
		int num_colors = 256;
		
		info->num_palette = num_colors;
		for (i = 0; i < num_colors; i++)
		{
			unsigned char pix = i;
			unsigned char c = 255 - i;
			info->palette[pix].red = c;
			info->palette[pix].green = c;
			info->palette[pix].blue = c;
		}
		rc = writepng_output(info);
		fclose(info->outfile);
	}
	writepng_exit(info);
	
	if (rc != 0)
	{
		g_string_append_printf(errorout, "error writing %s.png: %s\n", basename, strerror(rc));
		unlink(local_filename);
	}
		
	g_free(local_filename);

	return rc == 0;
}

/* ------------------------------------------------------------------------- */

#if 0
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
		info->have_bg = 0;
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
			unsigned char pix = i;
			unsigned char c = 255 - i;
			info->palette[pix].red = c;
			info->palette[pix].green = c;
			info->palette[pix].blue = c;
		}
		rc = writepng_output(info);
		fclose(info->outfile);
	}
	writepng_exit(info);
	
	if (target.pixel_mode != FT_PIXEL_MODE_NONE)
		FT_Bitmap_Done(ft_library, &target);
	
	if (rc != 0)
	{
		g_string_append_printf(errorout, "error writing %s.png: %s\n", c->basename, strerror(rc));
		unlink(c->local_filename);
	}
		
	return rc == 0;
}
#endif

/* ------------------------------------------------------------------------- */

static gboolean ft_make_char1(charinfo *c)
{
	FT_Glyph glyf;
	FT_GlyphSlot slot;
	FT_BitmapGlyph bitmapglyph;
	FT_Bitmap *source;
	unsigned int i, j;
	
	if (FT_Load_Glyph(face, c->char_index, load_flags | FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) != FT_Err_Ok)
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

	bitmapglyph = (FT_BitmapGlyph) glyf;
	source = &bitmapglyph->bitmap;

	for (i = 0; i < source->rows; i++)
	{
		for (j = 0; j < source->width; j++)
		{
			FT_UInt xpos, ypos;
			size_t ind;
			uint8_t *bits;
			uint8_t b;
			
			bits = source->buffer;
			b =	bits[i * source->pitch + (j / 8)];

			/*
			 * Output character to correct position in bitmap
			 */

			if (b & (1 << (7 - (j % 8))))
			{
				xpos = j - font_bb.lbearing + slot->bitmap_left;
				ypos = font_bb.ascent + i - slot->bitmap_top;
	

				if (xpos >= font_bb.width || ypos >= font_bb.height)
				{
					if (debug)
						g_string_append_printf(errorout, "char %x: position %dx%d outside bitmap %ux%u\n", c->char_id, xpos, ypos, font_bb.width, font_bb.height);
				} else
				{
#if 0
					ind = ypos * bytesPerRow;
					ind += xpos;
					char1.buffer[ind] = 255;
#endif
					ind = (c->char_id / PNG_GLYPHS_PER_ROW) * big_bytes_per_glyph_row + ypos * big_bytes_per_row * PNG_GLYPHS_PER_ROW;
					ind += (c->char_id % PNG_GLYPHS_PER_ROW) * big_bytes_per_row + xpos;
					big_buffer[ind] = 255;
				}
			}
		}
	}

	FT_Done_Glyph(glyf);
	
	return TRUE;
}

/* ------------------------------------------------------------------------- */

static gboolean ft_make_char8(charinfo *c)
{
	FT_Glyph glyf;
	FT_GlyphSlot slot;
	FT_BitmapGlyph bitmapglyph;
	FT_Bitmap *source;
	unsigned int i, j;
	int i_idx, j_idx;
	int coverage;
	
	if (FT_Load_Glyph(face, c->char_index, load_flags | FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) != FT_Err_Ok)
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

	bitmapglyph = (FT_BitmapGlyph) glyf;
	source = &bitmapglyph->bitmap;

	for (i = 0; i < source->rows; i += BPP8_MUL)
	{
		for (j = 0; j < source->width; j += BPP8_MUL)
		{
			FT_UInt xpos, ypos;
			size_t ind;

			coverage = 0;

			for (i_idx = 0; i_idx < BPP8_MUL && (i + i_idx) < source->rows; i_idx++)
			{
				for (j_idx = 0; j_idx < BPP8_MUL && (j + j_idx) < source->width; j_idx++)
				{
					uint8_t *bits = source->buffer;
					uint8_t b = bits[(i + i_idx) * source->pitch + ((j + j_idx) / 8)];

					if (b & (1 << (7 - ((j + j_idx) % 8))))
					{
						coverage++;
					}
				}
			}
			coverage = (255 * coverage) / 256;	/* need to be 0..255 range */

			/*
			 * Output character to correct position in bitmap
			 */

			xpos = (j / BPP8_MUL) - font_bb.lbearing + (slot->bitmap_left / BPP8_MUL);
			ypos = font_bb.ascent + (i / BPP8_MUL) - (slot->bitmap_top / BPP8_MUL);

			if (xpos >= font_bb.width || ypos >= font_bb.height)
			{
				if (debug)
					g_string_append_printf(errorout, "char %x: position %dx%d outside bitmap %ux%u\n", c->char_id, xpos, ypos, font_bb.width, font_bb.height);
			} else
			{
#if 0
				ind = ypos * bytesPerRow;
				ind += xpos;
				char8.buffer[ind] = coverage;
#endif
				ind = (c->char_id / PNG_GLYPHS_PER_ROW) * big_bytes_per_glyph_row + ypos * big_bytes_per_row * PNG_GLYPHS_PER_ROW;
				ind += (c->char_id % PNG_GLYPHS_PER_ROW) * big_bytes_per_row + xpos;
				big_buffer[ind] = coverage;
			}
		}
	}
	
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
	GString *background_css;
	size_t page_start;
	size_t line_start;
	gboolean any_defined_page, any_defined_line;
	
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

	infos = g_new0(charinfo, num_ids);
	if (infos == NULL)
	{
		g_string_append_printf(errorout, "%s\n", strerror(errno));
		return FALSE;
	}
		
	make_font_filename(fontfilename, fontname);
	font_info = get_font_info(face);
	
	t = time(NULL);
	tm = *gmtime(&t);
	basename = g_strdup_printf("%s_%04d%02d%02d%02d%02d%02d",
		fontfilename,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec);
	
	for (char_id = 0; char_id < num_ids; char_id++)
	{
		infos[char_id].char_index = 0;
		infos[char_id].char_id = UNDEFINED;
		infos[char_id].basename = NULL;
		infos[char_id].name[0] = '\0';
	}

	/*
	 * determine the average width of all the glyphs in the font
	 */
	{
		unsigned long total_width;
		uint16_t num_glyphs;
		glyphinfo_t max_bb;
		FT_Short average_width;
		FT_Int max_lbearing;
		
		total_width = 0;
		num_glyphs = 0;
		max_bb.width = 0;
		max_bb.height = 0;
		max_bb.ascent = 0;
		max_bb.descent = 0;
		max_bb.lbearing = 0;
		max_lbearing = 0;
		
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
					c->bbox.descent = 0;
					c->bbox.ascent = 0;
					c->bbox.width = 0;
					c->bbox.height = 0;
					c->bbox.lbearing = 0;
					c->bbox.rbearing = 0;
					c->bbox.advance = 0;
					if (FT_Load_Glyph(face, c->char_index, load_flags | FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == FT_Err_Ok)
					{
						FT_GlyphSlot slot;
						
						slot = face->glyph;
						c->bbox.descent = slot->bitmap.rows - slot->bitmap_top;
						c->bbox.ascent = slot->bitmap.rows - c->bbox.descent;
						c->bbox.width = slot->bitmap.width;
						c->bbox.height = slot->bitmap.rows;
						c->bbox.lbearing = slot->bitmap_left;
						c->bbox.advance = slot->advance.x;
						
						if (c->bbox.ascent > max_bb.ascent)
							max_bb.ascent = c->bbox.ascent;
				
						if (c->bbox.descent > max_bb.descent)
							max_bb.descent = c->bbox.descent;
				
						if (c->bbox.width > max_bb.width)
							max_bb.width = c->bbox.width;

						if (c->bbox.lbearing < max_bb.lbearing)
							max_bb.lbearing = c->bbox.lbearing;
						if (c->bbox.lbearing > max_lbearing)
							max_lbearing = c->bbox.lbearing;

						total_width += c->bbox.width;
						num_glyphs++;
					}
				}
			}
		}

		max_bb.width = (max_bb.width - max_bb.lbearing + max_lbearing + bpp_mul - 1) / bpp_mul;
		max_bb.ascent = (max_bb.ascent + bpp_mul - 1) / bpp_mul;
		max_bb.descent = (max_bb.descent + bpp_mul - 1) / bpp_mul;
		max_bb.height = max_bb.ascent + max_bb.descent + 1;
		max_bb.lbearing = -((-max_bb.lbearing + bpp_mul - 1) / bpp_mul);
		
		if (num_glyphs == 0)
			average_width = max_bb.width;
		else
			average_width = (((total_width + bpp_mul - 1) / bpp_mul) / num_glyphs);
		if (debug)
		{
			g_string_append_printf(errorout, "max height %d max width %d average width %d, ascent %d, descent %d, lbearing %d\n",
				max_bb.height, max_bb.width, average_width,
				max_bb.ascent,
				max_bb.descent,
				max_bb.lbearing);
		}
		
		font_bb = max_bb;
	}

	/*
	 * generate the CSS stylesheet for the background image
	 */
	background_css = g_string_new(NULL);
	g_string_append_printf(background_css, ".%s { background: url('%s/%s.png') no-repeat; }\n", glyph_bg_class, output_url, basename);
	for (char_id = 0; char_id < num_ids; char_id++)
	{
		c = &infos[char_id];
		if (c->char_id == UNDEFINED)
			continue;
		g_string_append_printf(background_css,
			"  .chr%04x { background-position: -%dpx -%dpx; width: %dpx; height: %dpx; }\n",
			char_id,
			(char_id % PNG_GLYPHS_PER_ROW) * font_bb.width,
			(char_id / PNG_GLYPHS_PER_ROW) * font_bb.height,
			font_bb.width, font_bb.height);
	}
	
	html_out_header(body, background_css, font_info, fontname, FALSE);
	g_string_free(font_info, TRUE);

	png_glyph_rows = (num_ids + PNG_GLYPHS_PER_ROW - 1) / PNG_GLYPHS_PER_ROW;
	
	big_bytes_per_row = font_bb.width;
	big_bytes_per_glyph_row = big_bytes_per_row * PNG_GLYPHS_PER_ROW * font_bb.height;
	big_buffer = g_new0(uint8_t, big_bytes_per_glyph_row * png_glyph_rows);
	
	if (debug)
	{
		g_string_append_printf(errorout, "ids: %u $%04x\n", num_ids, num_ids);
	}
	
	/*
	 * Render each character.
	 */
	for (char_id = 0; char_id < num_ids; char_id++)
	{
		c = &infos[char_id];
		if (c->char_id != UNDEFINED)
		{
			c->basename = basename;
			if (!(bitsPerPixel == 1 ? ft_make_char1(c) : ft_make_char8(c)))
			{
				g_string_append_printf(errorout, "can't make char 0x%x (0x%lx)\n", c->char_index, (unsigned long)char_id);
			}
		}
	}
	
	write_big_png(basename);
	g_free(big_buffer);
	
#if 0
	qsort(infos, num_chars, sizeof(infos[0]), cmp_info);
#endif
		
	/*
	 * generate the table
	 */
	g_string_append(body, "<table cellspacing=\"0\" cellpadding=\"0\">\n");
	page_start = body->len;
	any_defined_page = FALSE;
	for (id = 0; id < num_ids; id += CHAR_COLUMNS)
	{
		gboolean defined[CHAR_COLUMNS];
		const char *klass;
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
			if (c->char_id != UNDEFINED)
			{
				defined[j] = TRUE;
				any_defined_line |= TRUE;
				any_defined_page |= TRUE;
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
			
			if (defined[j])
			{
				uint32_t unicode;
				char *debuginfo = NULL;
				char *src;
				
				unicode = char_id;
				/* if (debug) */
				{
					debuginfo = g_strdup_printf("Width: %.3f Height %.3f&#10;Ascent: %.3f Descent: %.3f&#10;lb: %.3f adv: %.3f&#10;",
						(double)c->bbox.width / bpp_mul,
						(double)c->bbox.height / bpp_mul,
						(double)c->bbox.ascent / bpp_mul,
						(double)c->bbox.descent / bpp_mul,
						(double)c->bbox.lbearing / bpp_mul,
						(double)c->bbox.advance / (1 << 6) / bpp_mul);
				}
#if 0
				if (output_mode == MODE_OUTLINE)
					src = g_strdup_printf("<svg width=\"%d\" height=\"%d\"><use xlink:href=\"%s#chr%04lx\" style=\"text-align: left; vertical-align: top;\"></use></svg>",
						font_bb.width, font_bb.height,
						img[j], (unsigned long)char_id);
				else
					src = g_strdup_printf("<img alt=\"\" style=\"text-align: left; vertical-align: top; position: relative; left: %dpx; top: %dpx\" src=\"%s\">",
						0,
						0,
						img[j]);
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx; min-width: %dpx; min-height: %dpx;\" title=\""
					"Index: 0x%x (%u)&#10;"
					"Unicode: 0x%04lx &#%lu;&#10;"
					"%s&#10;%s"
					"\">%s</td>",
					font_bb.width, font_bb.height,
					font_bb.width, font_bb.height,
					c->char_index, c->char_index,
					(unsigned long)unicode, (unsigned long)unicode,
					ucd_get_name(unicode),
					debuginfo ? debuginfo : "",
					src);
#else
				src = g_strdup_printf("<div class=\"%s chr%04x\" style=\"text-align: left; vertical-align: top; position: relative; left: %dpx; top: %dpx; width: %dpx; height: %dpx;\"></div>",
					glyph_bg_class,
					char_id,
					0,
					0,
					font_bb.width, font_bb.height);
				g_string_append_printf(body,
					"<td class=\"spd_glyph_image\" style=\"width: %dpx; height: %dpx; min-width: %dpx; min-height: %dpx;\" title=\""
					"Index: 0x%x (%u)&#10;"
					"Unicode: 0x%04lx &#%lu;&#10;"
					"%s&#10;%s"
					"\">%s</td>",
					font_bb.width, font_bb.height,
					font_bb.width, font_bb.height,
					c->char_index, c->char_index,
					(unsigned long)unicode, (unsigned long)unicode,
					ucd_get_name(unicode),
					debuginfo ? debuginfo : "", src);
#endif
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
	
	/*
	 * load the font and set output character size.
	 */
	ft_error = FT_New_Face(ft_library, filename, face_index, &face);
	if (ft_error == FT_Err_Ok)
	{
		/*
		 * If DPI is not given, use pixes to specify the size.
		 */
		if (x_res > 0)
			ft_error = FT_Set_Char_Size(face, ((point_size / 10) << 6) * bpp_mul, 0, x_res, y_res);
		else
			ft_error = FT_Set_Pixel_Sizes(face, 0, (point_size / 10) * bpp_mul);
	}
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
	FT_Error ft_error;
	
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
	bitsPerPixel = quality >= 1 ? 8 : 1;
	bpp_mul = bitsPerPixel > 1 ? BPP8_MUL : 1;
	
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
	
	/*
	 * Initialize freetype library, load the font
	 * and set output character size.
	 */
	ft_error = FT_Init_FreeType(&ft_library);
	if (ft_error)
	{
		html_out_header(body, NULL, NULL, _("500 Internal Server Error"), TRUE);
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

	return retval;
}

/*
export SERVER_NAME=127.0.0.2
export REMOTE_ADDR=127.0.0.1
export HTTP_REFERER=http://127.0.0.2/spdview/
export SCRIPT_FILENAME=/srv/www/htdocs/spdview/ttfview.cgi
export SCRIPT_NAME=/spdview/ttfview.cgi
export REQUEST_METHOD=GET
export DOCUMENT_ROOT=/srv/www/htdocs
export QUERY_STRING='url=/spdview/ttffonts/Cantarell-Bold.otf&quality=1&points=120&resolution=95&debug=1'
*/
