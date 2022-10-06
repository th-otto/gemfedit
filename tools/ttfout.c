/*
 * ttfout.c - TTF converter.
 *
 * Written By:	Thorsten Otto
 */

#include "version.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "array.h"
#include "fontdef.h"
#include "ttnameid.h"
#include "tttables.h"

/* Count items in the array. */
#define arraysize(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a,b)    ((a) < (b) ? (a) : (b))

/* Required Tables */
#define CMAP	0
#define HEAD	1
#define HHEA	2
#define HMTX	3
#define MAXP	4
#define NAME	5
#define OS2		6
#define POST	7
/* Tables Related TrueType Outlines */
#define CVT	8
#define FPGM	9
#define GLYF	10
#define LOCA	11
#define PREP	12
/* Tables Related to Bitmap Glyphs */
#define EBDT	13
#define EBLC	14
#define EBSC	15
#define GASP	16
/* for the Apple QuickDraw GX */
#define BLOC	17
#define BDAT	18
#define BHED	19
/* Total table number */
#define NUM_TABLE 20

static const char *ttfTag[] = {
	/* Required tables. */
	"cmap", "head", "hhea", "hmtx", "maxp", "name", "OS/2", "post",
	/* for Vector glyph */
	"cvt ", "fpgm", "glyf", "loca", "prep",
	/* for Bitmap glyph */
	"EBDT", "EBLC", "EBSC",
	/* for Anti-aliasing */
	"gasp",
	/* for the Apple QuickDraw GX */
	"bloc", "bdat", "bhed"
};

static table *ttfTbl[NUM_TABLE];

/* prep program */
static unsigned char prep_prog[8] = {
	0xb9,
	1, 0xff, 0, 0,						/* push word 511, 0 */
	0x8d,								/* scan type */
	0x85,								/* scan ctrl */
	0x18,								/* round to grid */
};

struct ttf {
	int flagAutoName;
	int flagBold;
	int flagItalic;
	struct glyph *glyphs;
	int num_glyphs;
	int emX;
	int emY;
	int emDescent;
	int emAscent;
	int indexFirst;
	int indexLast;
};

static const size_t SIZE_BITMAPSIZETABLE = (sizeof(uint32_t) * 4
											+ sizeof(char[12]) * 2 + sizeof(uint16_t) * 2 + sizeof(char) * 4);
static const size_t SIZE_INDEXSUBTABLEARRAY = 8;
static const int EMMAX = 1024;
static const int MAX_SCALE = 100;

static const char *STYLE_REGULAR = "Regular";
static const char *STYLE_BOLD = "Bold";
static const char *STYLE_ITALIC = "Italic";
static const char *STYLE_BOLDITALIC = "Bold Italic";

const char *g_copyright = NULL;
const char *g_fontname = NULL;
const char *g_version = "1.0";
const char *g_trademark = NULL;

#define is_glyph_available(font, i) (font->glyphs[i].id >= 0)
#define get_glyph_width(font, i) font->glyphs[i].width
#define get_glyph_id(font, i) font->glyphs[i].id

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

static int emCalc(int pix, int base)
{
	return (EMMAX * pix + base / 2) / base;
}

/* -------------------------------------------------------------------------- */

static int order_sort(const void *a, const void *b)
{
	return strcmp(ttfTag[*(int *) a], ttfTag[*(int *) b]);
}

/* -------------------------------------------------------------------------- */

static void addbytes(array *p, const unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; ++i)
		array_addByte(p, buf[i]);
}

/* -------------------------------------------------------------------------- */

static int name_addstr(table *t, array *p, const char *buf, short platformID, short encodingID, short languageID, short nameID)
{
	int len;

	if (buf == NULL || *buf == '\0')
		return 0;
	len = (int)strlen(buf);
	array_addShort(t->ary, platformID);
	array_addShort(t->ary, encodingID);
	array_addShort(t->ary, languageID);
	array_addShort(t->ary, nameID);
	array_addShort(t->ary, (short) len);
	array_addShort(t->ary, array_getLen(p));
	addbytes(p, (const unsigned char *) buf, len);
	return 1;
}

/* -------------------------------------------------------------------------- */

static unichar_t utf8ptr_to_ucs4(const unsigned char *ptr, int *len)
{
	if ((ptr[0] & 0x80) == 0x00)
	{
		if (len)
			*len = 1;
		return (unichar_t) (ptr[0] & 0x7F);
	} else if ((ptr[0] & 0xE0) == 0xC0)
	{
		if (len)
			*len = 2;
		return (((unichar_t)ptr[0] & 0x1F) << 6) | ((unichar_t)ptr[1] & 0x3F);
	} else if ((ptr[0] & 0xF0) == 0xE0)
	{
		if (len)
			*len = 3;
		return (((unichar_t)ptr[0] & 0x0F) << 12) | (((unichar_t)ptr[1] & 0x3F) << 6) | ((unichar_t)ptr[2] & 0x3F);
	} else if ((ptr[0] & 0xF8) == 0xF0)
	{
		if (len)
			*len = 4;
		return (((unichar_t)ptr[0] & 0x07) << 18) |
			(((unichar_t)ptr[1] & 0x3F) << 12) |
			(((unichar_t)ptr[2] & 0x3F) << 6) |
			((unichar_t)ptr[3] & 0x3F);
	} else if ((ptr[0] & 0xFC) == 0xF8)
	{
		if (len)
			*len = 5;
		return (((unichar_t)ptr[0] & 0x03) << 24) |
			(((unichar_t)ptr[1] & 0x3F) << 18) |
			(((unichar_t)ptr[2] & 0x3F) << 12) |
			(((unichar_t)ptr[3] & 0x3F) << 6) |
			((unichar_t)ptr[4] & 0x3F);
	} else if ((ptr[0] & 0xFE) == 0xFC)
	{
		if (len)
			*len = 6;
		return (((unichar_t)ptr[0] & 0x01) << 30) |
			(((unichar_t)ptr[1] & 0x3F) << 24) |
			(((unichar_t)ptr[2] & 0x3F) << 18) |
			(((unichar_t)ptr[3] & 0x3F) << 12) |
			(((unichar_t)ptr[4] & 0x3F) << 6) |
			((unichar_t)ptr[5] & 0x3F);
	}

	if (len)
		*len = 1;
	return ' ';
}

/* -------------------------------------------------------------------------- */

static unichar_t *into_unicode(int *outlen, const char *buf)
{
	int unilen = 0;
	const char *p;
	unichar_t *pout;
	unichar_t *outbuf;

	for (p = buf; *p; ++unilen)
	{
		int len;

		utf8ptr_to_ucs4((const unsigned char *) p, &len);
		p += len;
	}
	outbuf = (unichar_t *) malloc(sizeof(unichar_t) * (unilen + 1));

	for (p = buf, pout = outbuf; *p; ++pout)
	{
		int len;

		*pout = utf8ptr_to_ucs4((const unsigned char *) p, &len);
		p += len;
	}
	*pout = 0;
	if (outlen)
		*outlen = unilen;
	return outbuf;
}

/* -------------------------------------------------------------------------- */

static int name_addunistr(table *t, array *p, const char *buf, short platformID, short encodingID, short languageID, short nameID)
{
	int unilen;
	unichar_t *unibuf;

	if (buf == NULL || *buf == '\0')
		return 0;
	array_addShort(t->ary, platformID);
	array_addShort(t->ary, encodingID);
	array_addShort(t->ary, languageID);
	array_addShort(t->ary, nameID);

	unibuf = into_unicode(&unilen, buf);
	unilen = unilen * 2;

	array_addShort(t->ary, (short) unilen);
	array_addShort(t->ary, array_getLen(p));
	array_addWideString(p, unibuf);
	free(unibuf);

	return 1;
}

/* -------------------------------------------------------------------------- */

static int search_topbit(unsigned long n)
{
	int topbit = 0;

	for (; n > 1UL; ++topbit)
		n >>= 1;
	return topbit;
}

/* -------------------------------------------------------------------------- */

static int get_pixel_glyph(struct font *font, struct glyph *glyph, int x, int y)
{
	if (x >= 0 && y >= 0 && x < glyph->bbx.width && y < glyph->bbx.height)
	{
		unsigned long offset = font->off_table[glyph->idx] + x;
		unsigned char *data = font->dat_table + y * font->form_width;

		return data[(offset >> 3)] & (0x80 >> (int)(offset & 7)) ? 1 : 0;
	}
	return -1;
}

/* -------------------------------------------------------------------------- */

static unsigned char *get_glyph_bitmap(struct font *font, struct glyph *glyph)
{
	unsigned char *buf;
	unsigned char *p;
	unsigned char mask = 0x00;
	int w = glyph->bbx.width;
	int h = glyph->bbx.height;
	int bufsize = (w * h + 7) >> 3;
	int x, y;
	
	buf = (unsigned char *) calloc(1, bufsize);

	p = buf - 1;

	for (y = 0; y < h; ++y)
	{
		for (x = 0; x < w; ++x)
		{
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80;
				++p;
			}
			if (get_pixel_glyph(font, glyph, x, y) > 0)
				*p |= mask;
		}
	}
	return buf;
}

/* -------------------------------------------------------------------------- */

static void add_bigGlyphMetrics(array *t, int height, int width,
	int horiBearingX, int horiBearingY, int horiAdvance,
	int vertBearingX, int vertBearingY, int vertAdvance)
{
	array_addByte(t, height);
	array_addByte(t, width);
	array_addByte(t, horiBearingX);
	array_addByte(t, horiBearingY);
	array_addByte(t, horiAdvance);
	array_addByte(t, vertBearingX);
	array_addByte(t, vertBearingY);
	array_addByte(t, vertAdvance);
}

/* -------------------------------------------------------------------------- */

#if 0
/*
 * Reference implementation
 */
static int generate_eb_rawbitmap(bdf_t *bdf, table *ebdt, array *st)
{
	int len;
	int i;
	
	array_addShort(st, 1);					/* firstGlyphIndex */
	array_addShort(st, bdf->numGlyph);		/* lastGlyphIndex */
	array_addLong(st, sizeof(uint32_t) * 1 + sizeof(uint16_t) * 2);
	/* additionalOffsetToIndexSubtable */
	/* indexSubTable #0 */
	array_addShort(st, 1);					/* indexFormat */
	array_addShort(st, 6);					/* imageFormat */
	array_addLong(st, 0);					/* imageDataOffset */
	array_addLong(st, array_getLen(ebdt->ary));

	for (i = 0; i < MAX_GLYPH; ++i)
	{
		bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
		int gwidth, gheight;

		if (!glyph)
			continue;
		gwidth = MIN(glyph->bbx.width, bdf->bbx.width);
		gheight = MIN(glyph->bbx.height, bdf->bbx.height);

		add_bigGlyphMetrics(ebdt, gheight, gwidth, 0, gheight + glyph->bbx.offset.y, gwidth, -gwidth / 2, 0, gheight);

		len = ((gwidth + 7) >> 3) * gheight;
		addbytes(ebdt, glyph->bitmap, len);
		array_addLong(st->ary, array_getLen(ebdt->ary));
	}

	return 1;
}
#endif

/* -------------------------------------------------------------------------- */

static int count_subtable(struct font *font)
{
	int n_subtbl = 0;
	long i;
	
	for (i = 0; i < MAX_GLYPH; ++i)
	{
		struct boundingbox bbx;
		long j;

		if (!is_glyph_available(font, i))
			continue;

		bbx = font->glyphs[i].bbx;

		for (j = i + 1; j < MAX_GLYPH; ++j)
		{
			if (!is_glyph_available(font, j))
				continue;
			if (memcmp(&bbx, &font->glyphs[j].bbx, sizeof(bbx)) != 0)
				break;
		}
		i = j - 1;
		++n_subtbl;
	}
	return n_subtbl;
}

/* -------------------------------------------------------------------------- */

static int generate_eb_optbitmap(struct ttf *ttf, struct font *font, table *ebdt, array *st)
{
	int n_subtbl = count_subtable(font);
	long i;

	/* for indexFormat 2, imageFormat 5 */
	{
		array *sh = array_new();

		for (i = 0; i < MAX_GLYPH; ++i)
		{
			int idStart;
			int idEnd;
			long encStart;
			long encEnd;
			struct boundingbox bbx;
			long j;
			int w, h, imgsize;

			if (!is_glyph_available(font, i))
				continue;

			bbx = font->glyphs[i].bbx;
			idStart = idEnd = get_glyph_id(ttf, i);
			encStart = encEnd = i;
			
			for (j = i + 1; j < MAX_GLYPH; ++j)
			{
				if (!is_glyph_available(font, j))
					continue;
				if (memcmp(&bbx, &font->glyphs[j].bbx, sizeof(bbx)) != 0)
					break;
				idEnd = get_glyph_id(font, j);
				encEnd = j;
			}
			i = j - 1;

			w = MIN(bbx.width, font->max_cell_width);
			h = MIN(bbx.height, font->form_height);
			imgsize = (w * h + 7) >> 3;

			/* indexSubTableArray */
			array_addShort(st, idStart + 1);	/* firstGlyphIndex */
			array_addShort(st, idEnd + 1);		/* lastGlyphIndex */
			array_addLong(st, array_getLen(sh) + SIZE_INDEXSUBTABLEARRAY * n_subtbl);
			/* additionalOffsetToIndexSubtable */

			/* indexSubTable format #2 */
			/* indexSubHeader */
			array_addShort(sh, 2);			/* indexFormat; monospaced */
			array_addShort(sh, 5);			/* imageFormat */
			array_addLong(sh, array_getLen(ebdt->ary));	/* imageDataOffset */
			/* table body */
			array_addLong(sh, imgsize);		/* imageSize */
			/* bigMetrics */
#if 0
			glyph = bdf_get_glyph(bdf, encStart);
			add_bigGlyphMetrics(sh, h, w, 0, h + glyph->bbx.offset.y, w, -w / 2, 0, h);
#else
			add_bigGlyphMetrics(sh, h, w, 0, h, w, -w / 2, 0, h);
#endif

			for (j = encStart; j <= encEnd; ++j)
			{
				if (!is_glyph_available(font, j))
				{
					int k;
					
					if (is_glyph_available(ttf, j))
						for (k = 0; k < imgsize; ++k)
							array_addByte(ebdt->ary, 0);
				} else
				{
					unsigned char *buf = get_glyph_bitmap(font, &font->glyphs[j]);

					addbytes(ebdt->ary, buf, imgsize);
					free(buf);
				}
			}
		}
		array_addArray(st, sh);
		array_delete(sh);
	}

	return n_subtbl;
}

/* -------------------------------------------------------------------------- */

static void add_sbitLineMetric(array *t,
	int ascender, int descender, int widthMax,
	int caretSlopeNumerator, int caretSlopeDenominator, int caretOffset,
	int minOriginSB, int minAdvanceSB, int maxBeforeBL, int minAfterBL, int pad1, int pad2)
{
	array_addByte(t, ascender);
	array_addByte(t, descender);
	array_addByte(t, widthMax);
	array_addByte(t, caretSlopeNumerator);
	array_addByte(t, caretSlopeDenominator);
	array_addByte(t, caretOffset);
	array_addByte(t, minOriginSB);
	array_addByte(t, minAdvanceSB);
	array_addByte(t, maxBeforeBL);
	array_addByte(t, minAfterBL);
	array_addByte(t, pad1);
	array_addByte(t, pad2);
}

/* -------------------------------------------------------------------------- */

static void generate_eb_location(struct ttf *ttf,
	table *eblc, array *subtable, array *starray,
	int n_plane, int n_subtbl, int width, int height, int origsize)
{
	int a, d, s;
	int n_glyph;
	
	/* bitmapSizeTable */
	array_addLong(eblc->ary, sizeof(uint32_t) * 2 + SIZE_BITMAPSIZETABLE * n_plane
				  + array_getLen(starray));
	/* indexSubTableArrayOffset */
	array_addLong(eblc->ary, array_getLen(subtable));	/* indexTableSize */
	array_addLong(eblc->ary, n_subtbl);			/* numberOfIndexSubTables */
	array_addLong(eblc->ary, 0);					/* colorRef */
	/* sbitLineMetrics: hori */
#if 0
	a = bdf->ascent;
	d = -bdf->descent;
#else
	a = width;
	d = height;
#endif
	s = origsize;

	add_sbitLineMetric(eblc->ary, a, d, s, 1, 0, 0, 0, s, a, d, 0, 0);
	/* sbitLineMetrics: vert */
	add_sbitLineMetric(eblc->ary, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	array_addShort(eblc->ary, 1);					/* startGlyphIndex */
	n_glyph = ttf->num_glyphs;

	array_addShort(eblc->ary, n_glyph);			/* endGlyphIndex */
	array_addByte(eblc->ary, origsize);			/* ppemX */
	array_addByte(eblc->ary, origsize);			/* ppemY */
	array_addByte(eblc->ary, 1);				/* bitDepth */
	array_addByte(eblc->ary, 0x01);				/* flags: */
	/*  0x01=Horizontal */
	/*  0x02=Vertical */
}

/* -------------------------------------------------------------------------- */

static void add_scaletable(table *ebsc, int targetsize, int width, int height, int orig)
{
	int size = targetsize;
	int hsize;
	
	if (size == orig)
		return;
	hsize = (targetsize * height + width / 2) / width;

	/* sbitLineMetrics: hori */
	add_sbitLineMetric(ebsc->ary, 0, 0, targetsize, 0, 0, 0, 0, 0, hsize, 0, 0, 0);
	/* sbitLineMetrics: vert */
	add_sbitLineMetric(ebsc->ary, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	array_addByte(ebsc->ary, targetsize);
	array_addByte(ebsc->ary, targetsize);
	array_addByte(ebsc->ary, orig);
	array_addByte(ebsc->ary, orig);
}

/* -------------------------------------------------------------------------- */

static void generate_EBDT_EBLC_EBSC(struct ttf *ttf, struct font **fonts, int num_fonts)
{
	int scsize = 1;
	array *starray;
	int i;

	table *ebdt = ttfTbl[EBDT];
	table *eblc = ttfTbl[EBLC];
	table *ebsc = ttfTbl[EBSC];

	array_addLong(ebdt->ary, 0x00020000L);
	array_addLong(eblc->ary, 0x00020000L);
	array_addLong(eblc->ary, num_fonts);
	array_addLong(ebsc->ary, 0x00020000L);				/* version */
	array_addLong(ebsc->ary, MAX_SCALE - num_fonts);	/* numSizes */

	starray = array_new();

	for (i = 0; i < num_fonts; ++i)
	{
		struct font *font = fonts[i];
		array *subtable;
		int w, h;
		int orig;
		int n_subtable;
		int nextsize;
		
		w = font->max_cell_width;
		h = font->form_height;

		orig = font->form_height;		/*(w > h / 2) ? w : w * 2; */

		subtable = array_new();
		n_subtable = generate_eb_optbitmap(ttf, font, ebdt, subtable);
		generate_eb_location(ttf, eblc, subtable, starray, num_fonts, n_subtable, w, h, orig);
		array_addArray(starray, subtable);

		nextsize = (i < num_fonts - 1) ? fonts[i + 1]->form_height : MAX_SCALE;

		for (; scsize <= nextsize; ++scsize)
			add_scaletable(ebsc, scsize, w, h, orig);
		
		array_delete(subtable);
	}

	table_calcSum(ebdt);
	array_addArray(eblc->ary, starray);
	table_calcSum(eblc);
	table_calcSum(ebsc);
	
	array_delete(starray);
}

/* -------------------------------------------------------------------------- */

static void glyph_simple(struct ttf *ttf, array *glyf)
{
	array_addShort(glyf, 1);					/* numberOfContours */
	array_addShort(glyf, 0);					/* xMin */
	array_addShort(glyf, -ttf->emDescent);		/* yMin */
	array_addShort(glyf, ttf->emX / 2);			/* xMax */
	/* hmtx define the kerning, not here */
	array_addShort(glyf, ttf->emAscent);		/* yMax */

	array_addShort(glyf, 0);					/* endPtsOfContours[n] */
	array_addShort(glyf, 0);					/* instructionLength */
	array_addByte(glyf, 0x37);					/* flag[0]: On curve, both x,y are positive short */
	array_addByte(glyf, 1);						/* xCoordinates[] */
	array_addByte(glyf, 1);						/* yCoordinates[] */
	array_addByte(glyf, 0);						/* pad */
}

/* -------------------------------------------------------------------------- */

static void glyph_empty(array *glyf)
{
	array_addShort(glyf, 0);					/* numberOfContours */
	array_addShort(glyf, 0);					/* xMin */
	array_addShort(glyf, 0);					/* yMin */
	array_addShort(glyf, 0);					/* xMax */
	array_addShort(glyf, 0);					/* yMax */
	array_addShort(glyf, 0);					/* instructionLength */
}

/* -------------------------------------------------------------------------- */

static void generate_LOCA_GLYF(struct ttf *ttf)
{
	table *loca = ttfTbl[LOCA];
	table *glyf = ttfTbl[GLYF];
	int remain;
	long i;
	size_t empty_off;

	/* #0 */
	array_addLong(loca->ary, 0);
	glyph_simple(ttf, glyf->ary);
	empty_off = array_getLen(glyf->ary);

	remain = ttf->num_glyphs;

	for (i = 0; i < MAX_GLYPH; ++i)
	{
		if (is_glyph_available(ttf, i))
		{
			array_addLong(loca->ary, empty_off);
			--remain;
		}
	}
	/*
	 * add extra offset for idnex #num_glyphs,
	 * to be able to calculate size from offset[index + ] - offset[index]
	 */
	array_addLong(loca->ary, empty_off);
	glyph_empty(glyf->ary);

	table_calcSum(glyf);
	table_calcSum(loca);
}

/* -------------------------------------------------------------------------- */

static void generate_CMAP(struct font *font)
{
	array *fmat0 = array_new();
	array *fmat4 = array_new();
	int numtbl;
	long offset;
	table *cmap;

	/* Make format 0 */
	{
		int i;

		array_addShort(fmat0, 0);				/* format */
		array_addShort(fmat0, sizeof(uint16_t) * 3 + 256);
		/* len */
		array_addShort(fmat0, TT_MAC_LANGID_ENGLISH);				/* language */
		for (i = 0; i < 0x100; ++i)
		{
			int gid = is_glyph_available(font, i) ? font->glyphs[i].id + 1 : 0;

			array_addByte(fmat0, gid);
		}
	}

	/* Make format 4 */
	{
		array *endCount = array_new();
		array *startCount = array_new();
		array *idDeleta = array_new();
		array *idRangeOffset = array_new();
		int segCount = 0;
		long i, j;
		unsigned short segCountX2;
		unsigned short entrySelector;
		unsigned short searchRange;
		
		for (i = 0; i < MAX_GLYPH; ++i)
		{
			if (!is_glyph_available(font, i))
			{
				if (i == MAX_GLYPH - 1)
				{
					++segCount;
					array_addShort(endCount, MAX_GLYPH - 1);
					array_addShort(startCount, MAX_GLYPH - 1);
					array_addShort(idDeleta, 1);
					array_addShort(idRangeOffset, 0);
				}
				continue;
			}
			j = i + 1;

			while (is_glyph_available(font, j))
				++j;
			j -= 1;
			array_addShort(endCount, j);
			array_addShort(startCount, i);
			array_addShort(idDeleta, font->glyphs[i].id - i + 1);
			array_addShort(idRangeOffset, 0);
			++segCount;
			i = j + 1;
		}

		segCountX2 = segCount * 2;
		entrySelector = search_topbit(segCount);
		searchRange = 1 << (entrySelector + 1);

		array_addShort(fmat4, 4);				/* format */
		array_addShort(fmat4, sizeof(uint16_t) * 8 + segCount * sizeof(uint16_t) * 4);
		/* length */
		array_addShort(fmat4, TT_MAC_LANGID_ENGLISH);				/* language */
		array_addShort(fmat4, segCountX2);		/* segCountX2 */
		array_addShort(fmat4, searchRange);		/* searchRange */
		array_addShort(fmat4, entrySelector);	/* entrySelector */
		array_addShort(fmat4, segCountX2 - searchRange);
		/* rangeShift */
		array_addArray(fmat4, endCount);
		array_addShort(fmat4, 0);				/* reservedPad */
		array_addArray(fmat4, startCount);
		array_addArray(fmat4, idDeleta);
		array_addArray(fmat4, idRangeOffset);

		array_delete(endCount);
		array_delete(startCount);
		array_delete(idDeleta);
		array_delete(idRangeOffset);
	}

	/* follow make cmap */
#ifdef USE_CMAP_3RD_TABLE
	/* for 3rd table */
	numtbl = 3;
#else
	numtbl = 2;
#endif
	offset = sizeof(uint16_t) * 2 + (sizeof(uint16_t) * 2 + sizeof(uint32_t)) * numtbl;

	cmap = ttfTbl[CMAP];

	array_addShort(cmap->ary, 0);						/* Version */
	array_addShort(cmap->ary, numtbl);					/* #tbl */
	/* Table 1 (format 0) */
	array_addShort(cmap->ary, TT_PLATFORM_MACINTOSH);	/* Platform ID = 1 -> Macintosh */
	array_addShort(cmap->ary, TT_MAC_ID_ROMAN);			/* Encoding ID = 0 -> Roman */
	array_addLong(cmap->ary, offset);
	/* Table 2 (format 4) */
	array_addShort(cmap->ary, TT_PLATFORM_MICROSOFT);	/* Platform ID = 3 -> Microsoft */
	array_addShort(cmap->ary, TT_MS_ID_UNICODE_CS);		/* Encoding ID = 1 -> Unicode */
	array_addLong(cmap->ary, offset + array_getLen(fmat0));
#if USE_CMAP_3RD_TABLE
	/* Table 3 (format X) */
	array_addShort(cmap->ary, 0);						/* Platform ID = X -> Unknown */
	array_addShort(cmap->ary, 0);						/* Encoding ID = X -> Unknown */
	array_addLong(cmap->ary, offset + array_getLen(fmat0));
#endif

	/* Add table main data stream */
	array_addArray(cmap->ary, fmat0);
	array_addArray(cmap->ary, fmat4);

	table_calcSum(cmap);
	
	array_delete(fmat0);
	array_delete(fmat4);
}

/* -------------------------------------------------------------------------- */

static void generate_OS2_PREP(struct ttf *ttf)
{
	short fsSelection = 0x0000;
	short usWeightClass = 400;
	char bWeight = 5;
	table *os2;
	
	if (ttf->flagBold)
	{
		fsSelection |= 0x0020;
		usWeightClass = 700;
		bWeight = 8;
	}
	if (ttf->flagItalic)
		fsSelection |= 0x0001;
	if (!ttf->flagItalic && !ttf->flagBold)
		fsSelection |= 0x0040; /* Regular */

	/* Generate OS/2 and PREP */
	os2 = ttfTbl[OS2];

	array_addShort(os2->ary, 0x0001);				/* version */
	array_addShort(os2->ary, ttf->emX / 2);		/* xAvgCharWidth */
	array_addShort(os2->ary, usWeightClass);		/* usWeightClass */
	array_addShort(os2->ary, 5);					/* usWidthClass */
	array_addShort(os2->ary, 0x0004);				/* fsType */
	array_addShort(os2->ary, 512);					/* ySubscriptXSize */
	array_addShort(os2->ary, 512);					/* ySubscriptYSize */
	array_addShort(os2->ary, 0);					/* ySubscriptXOffset */
	array_addShort(os2->ary, 0);					/* ySubscriptYOffset */
	array_addShort(os2->ary, 512);					/* ySuperscriptXSize */
	array_addShort(os2->ary, 512);					/* ySuperscriptYSize */
	array_addShort(os2->ary, 0);					/* ySuperscriptXOffset */
	array_addShort(os2->ary, 512);					/* ySuperscriptYOffset */
	array_addShort(os2->ary, 51);					/* yStrikeoutSize */
	array_addShort(os2->ary, 260);					/* yStrikeoutPosition */
	array_addShort(os2->ary, 0);					/* sFamilyClass */
	array_addByte(os2->ary, 2);						/* panose.bFamilyType */
	array_addByte(os2->ary, 0);						/* panose.bSerifStyle */
	array_addByte(os2->ary, bWeight);				/* panose.bWeight */
	array_addByte(os2->ary, 9);						/* panose.bProportion */
	array_addByte(os2->ary, 0);						/* panose.bContrast */
	array_addByte(os2->ary, 0);						/* panose.bStrokeVariation */
	array_addByte(os2->ary, 0);						/* panose.bArmStyle */
	array_addByte(os2->ary, 0);						/* panose.bLetterform */
	array_addByte(os2->ary, 0);						/* panose.bMidline */
	array_addByte(os2->ary, 0);						/* panose.bXHeight */
	/* TODO  */
	array_addLong(os2->ary, 0xffffffffUL);			/* ulUnicodeRange1 */
	array_addLong(os2->ary, 0xffffffffUL);			/* ulUnicodeRange2 */
	array_addLong(os2->ary, 0xffffffffUL);			/* ulUnicodeRange3 */
	array_addLong(os2->ary, 0xffffffffUL);			/* ulUnicodeRange4 */
	array_addString(os2->ary, "KRN ");				/* achVendID */
	array_addShort(os2->ary, fsSelection);			/* fsSelection */
	array_addShort(os2->ary, ttf->indexFirst);		/* usFirstCharIndex */
	array_addShort(os2->ary, ttf->indexLast);		/* usLastCharIndex */

	array_addShort(os2->ary, ttf->emAscent);		/* sTypoAscender */
	array_addShort(os2->ary, -ttf->emDescent);		/* sTypoDescender */
	array_addShort(os2->ary, 0);					/* sTypoLineGap */
	array_addShort(os2->ary, ttf->emAscent);		/* usWinAscent */
	array_addShort(os2->ary, ttf->emDescent);		/* usWinDesent */
#if 1
	/* TODO */
#if 0
	array_addLong(os2->ary, 0x4002000FUL);			/* ulCodePageRange1 */
	array_addLong(os2->ary, 0xD2000000UL);			/* ulCodePageRange2 */
#else
	array_addLong(os2->ary, 0xffffffffUL);			/* ulCodePageRange1 */
	array_addLong(os2->ary, 0xffffffffUL);			/* ulCodePageRange2 */
#endif
#endif
#if 0
	/* For version 2. */
	array_addShort(os2->ary, 0x0040);				/* sxHeight */
	array_addShort(os2->ary, 0x0040);				/* sCapHeight */
	array_addShort(os2->ary, 0x0040);				/* usDefaultChar */
	array_addShort(os2->ary, 0x0040);				/* usBreakChar */
	array_addShort(os2->ary, 0x0040);				/* usMaxContext */
#endif
	table_calcSum(os2);

	addbytes(ttfTbl[PREP]->ary, &prep_prog[0], (int)arraysize(prep_prog));
	table_calcSum(ttfTbl[PREP]);
}

/* -------------------------------------------------------------------------- */

static void generate_GASP(struct font *font)
{
	/*int size = MIN(font->width, font->height); */
	table *gasp = ttfTbl[GASP];

	(void)font;
	array_addShort(gasp->ary, 0);					/* version */
#if 0
	if (size < 16)
		array_addShort(gasp->ary, 3);
	else
		array_addShort(gasp->ary, 2);

	array_addShort(gasp->ary, size - 1);
	array_addShort(gasp->ary, 0x0002);
	if (size < 16)
	{
		array_addShort(gasp->ary, 16);
		array_addShort(gasp->ary, 0x0001);
	}
	array_addShort(gasp->ary, 0xffff);
	array_addShort(gasp->ary, 0x0003);
#else
	array_addShort(gasp->ary, 1);					/* numRange */
	array_addShort(gasp->ary, 0xffff);				/* rangeMaxPPEM */
	array_addShort(gasp->ary, 0x0001);				/* rangeGaspBehavior */
#endif

	table_calcSum(gasp);
}

/* -------------------------------------------------------------------------- */

static void generate_HEAD(struct ttf *ttf)
{
	short macStyle = 0x0000;
	table *tmp;
	time_t t;

	if (ttf->flagBold)
		macStyle |= 0x0001;
	if (ttf->flagItalic)
		macStyle |= 0x0002;

	tmp = ttfTbl[HEAD];

	array_addLong(tmp->ary, 0x10000UL);				/* version */
	array_addLong(tmp->ary, 0x10000UL);				/* fontRevision */
	array_addLong(tmp->ary, 0);						/* checkSumAdjustment */
	array_addLong(tmp->ary, 0x5f0f3cf5UL);			/* magicNumber */
	array_addShort(tmp->ary, 0x000b);				/* flags */
	array_addShort(tmp->ary, EMMAX);				/* unitPerEm */
	t = time(NULL);
	t += 2082844800L;
#ifdef __PUREC__
	array_addLong(tmp->ary, 0);						/* createdTime */
	array_addLong(tmp->ary, t);						/* createdTime */
	array_addLong(tmp->ary, 0);						/* modifiedTime */
	array_addLong(tmp->ary, t);						/* modifiedTime */
#else
	array_addQuad(tmp->ary, t);						/* createdTime */
	array_addQuad(tmp->ary, t);						/* modifiedTime */
#endif
	array_addShort(tmp->ary, 0);					/* xMin */
	array_addShort(tmp->ary, -ttf->emDescent);		/* yMin */
	array_addShort(tmp->ary, ttf->emX);			/* xMax */
	array_addShort(tmp->ary, ttf->emAscent);		/* yMax */
	array_addShort(tmp->ary, macStyle);				/* macStyle */
	array_addShort(tmp->ary, 7);					/* lowestRecPPEM */
	array_addShort(tmp->ary, 1);					/* fontDirectionHint */
	array_addShort(tmp->ary, 1);					/* indexToLocFormat */
	array_addShort(tmp->ary, 0);					/* glyphDataFormat */
	table_calcSum(tmp);
}

/* -------------------------------------------------------------------------- */

static void generate_HHEA(struct ttf *ttf)
{
	table *tmp;

	tmp = ttfTbl[HHEA];
	array_addLong(tmp->ary, 0x10000UL);				/* version */
	array_addShort(tmp->ary, ttf->emAscent);		/* Ascender */
	array_addShort(tmp->ary, -ttf->emDescent);		/* Descender */
	array_addShort(tmp->ary, 0);					/* yLineGap */
	array_addShort(tmp->ary, ttf->emX);				/* advanceWidthMax */
	array_addShort(tmp->ary, 0);					/* minLeftSideBearing */
	array_addShort(tmp->ary, 0);					/* minRightSideBearing */
	array_addShort(tmp->ary, ttf->emX);				/* xMaxExtent */
	array_addShort(tmp->ary, 1);					/* caretSlopeRise */
	array_addShort(tmp->ary, 0);					/* caretSlopeRun */
	array_addShort(tmp->ary, 0);					/* caretOffset */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* metricDataFormat */
	array_addShort(tmp->ary, ttf->num_glyphs + 1);	/* numberOfHMetrics */
	table_calcSum(tmp);
}

/* -------------------------------------------------------------------------- */

static void generate_HMTX(struct font *font)
{
	table *tmp;
	long i;

	tmp = ttfTbl[HMTX];
	/* #0 */
	array_addShort(tmp->ary, emCalc(1, font->form_height));
	array_addShort(tmp->ary, 0);
	for (i = 0; i < MAX_GLYPH; ++i)
	{
		if (!is_glyph_available(font, i))
			continue;
		array_addShort(tmp->ary, emCalc(get_glyph_width(font, i), font->form_height)); /* advanceWidth */
		array_addShort(tmp->ary, 0);                                   /* leftSideBearing */
	}
	table_calcSum(tmp);
}

/* -------------------------------------------------------------------------- */

static void generate_MAXP(struct font *font)
{
	table *tmp;

	tmp = ttfTbl[MAXP];
	array_addLong(tmp->ary, 0x10000UL);				/* Version */
	array_addShort(tmp->ary, font->num_glyphs + 1);	/* numGlyphs */
	array_addShort(tmp->ary, 800);					/* maxPoints */
	array_addShort(tmp->ary, 40);					/* maxContours */
	array_addShort(tmp->ary, 800);					/* maxCompositePoints */
	array_addShort(tmp->ary, 40);					/* maxCompositeContours */
	array_addShort(tmp->ary, 2);					/* maxZones */
	array_addShort(tmp->ary, 0);					/* maxTwilightPoints */
	array_addShort(tmp->ary, 10);					/* maxStorage */
	array_addShort(tmp->ary, 0);					/* maxFunctionDefs */
	array_addShort(tmp->ary, 0);					/* maxInstructionDefs */
	array_addShort(tmp->ary, 1024);					/* maxStackElements */
	array_addShort(tmp->ary, 0);					/* maxSizeOfInstructions */
	array_addShort(tmp->ary, 1);					/* maxComponentElements */
	array_addShort(tmp->ary, 1);					/* maxComponentDepth */
	table_calcSum(tmp);
}

/* -------------------------------------------------------------------------- */

static void generate_POST(struct ttf *ttf, struct font *font)
{
	table *tmp;

	tmp = ttfTbl[POST];
	array_addLong(tmp->ary, 0x30000UL);				/* Version */
	array_addLong(tmp->ary, 0);						/* italicAngle */
	array_addShort(tmp->ary, -ttf->emDescent);		/* underlinePosition */
	array_addShort(tmp->ary, font->ul_size);		/* underlineThickness */
	array_addLong(tmp->ary, font->flags & F_MONOSPACE ? 1 : 0);						/* isFixedPitch */
	array_addLong(tmp->ary, 0);						/* minMemType42 */
	array_addLong(tmp->ary, 0);						/* maxMemType42 */
	array_addLong(tmp->ary, 0);						/* minMemType1 */
	array_addLong(tmp->ary, 0);						/* maxMemType1 */
	table_calcSum(tmp);
}

/* -------------------------------------------------------------------------- */

static void generate_NAME(struct ttf *ttf, struct font *font)
{
	int num = 0;
	array *p = array_new();
	table *t = ttfTbl[NAME];
	char fullname[1024];
	const char *g_style;
	const char *fontname = g_fontname ? g_fontname : font->name;

	g_style = STYLE_REGULAR;
	if (ttf->flagAutoName)
	{
		if (ttf->flagBold && ttf->flagItalic)
			g_style = STYLE_BOLDITALIC;
		else if (ttf->flagBold)
			g_style = STYLE_BOLD;
		else if (ttf->flagItalic)
			g_style = STYLE_ITALIC;
	}

	strcpy(fullname, fontname);
	if (g_style != STYLE_REGULAR)
	{
		strcat(fullname, " ");
		strcat(fullname, g_style);
	}

	array_addShort(t->ary, 0);						/* format selector */
	array_addShort(t->ary, num);					/* #NameRec */
	array_addShort(t->ary, 6 + num * 12);			/* Offset of strings */

	num += name_addstr(t, p, g_copyright, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_COPYRIGHT);
	num += name_addstr(t, p, fontname, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FONT_FAMILY);
	num += name_addstr(t, p, g_style, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FONT_SUBFAMILY);
	num += name_addstr(t, p, fontname, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_UNIQUE_ID);
	num += name_addstr(t, p, fullname, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FULL_NAME);
	num += name_addstr(t, p, g_version, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_VERSION_STRING);
	num += name_addstr(t, p, fontname, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_PS_NAME);
	num += name_addstr(t, p, g_trademark, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_TRADEMARK);

	if (array_getLen(p) & 1)
		array_addByte(p, 0);

	num += name_addunistr(t, p, g_copyright, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_COPYRIGHT);
	num += name_addunistr(t, p, fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FONT_FAMILY);
	num += name_addunistr(t, p, g_style, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FONT_SUBFAMILY);
	num += name_addunistr(t, p, fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_UNIQUE_ID);
	num += name_addunistr(t, p, fullname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FULL_NAME);
	num += name_addunistr(t, p, g_version, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_VERSION_STRING);
	num += name_addunistr(t, p, fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_PS_NAME);
	num += name_addunistr(t, p, g_trademark, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_TRADEMARK);

	array_addArray(t->ary, p);
	array_setShort(t->ary, 2, num);					/* #NameRec */
	array_setShort(t->ary, 4, 6 + num * 12);		/* Offset of strings */

	table_calcSum(t);
	
	array_delete(p);
}

/* -------------------------------------------------------------------------- */

static void write_all_tables(FILE *fn)
{
	int i;

	/* tableOrder */
	static const int order[] = {
		OS2, CMAP, HEAD, HHEA, HMTX, MAXP, NAME, POST,
		EBLC, EBDT, EBSC,
		GASP,
		/* CVT, FPGM, */
		PREP, LOCA, GLYF,
		BLOC, BDAT, BHED
	};
	table *hob = table_new();
	uint32_t sum = 0;
	int numTbl = (int)arraysize(order);
	int order2[arraysize(order)];

	array_addLong(hob->ary, 0x10000UL);				/* Version */
	array_addShort(hob->ary, numTbl);				/* number of tables */
	array_addShort(hob->ary, 128);					/* search range */
	array_addShort(hob->ary, 3);					/* entry selector */
	array_addShort(hob->ary, numTbl * SIZEOF_TT_tablerec - 128);	/* range shift */

	ttfTbl[order[0]]->off = SIZEOF_TT_fileheader + numTbl * SIZEOF_TT_tablerec;
	for (i = 1; i < numTbl; ++i)
	{
		int prev = order[i - 1];

		ttfTbl[order[i]]->off = ttfTbl[prev]->off + table_getTableLen(ttfTbl[prev]);
	}

	memcpy(order2, order, sizeof(order));
	qsort(order2, numTbl, sizeof(order2[0]), order_sort);
	for (i = 0; i < numTbl; i++)
	{
		int ntbl = order2[i];
		table *mapTo;
		
		array_addByte(hob->ary, ttfTag[ntbl][0]);
		array_addByte(hob->ary, ttfTag[ntbl][1]);
		array_addByte(hob->ary, ttfTag[ntbl][2]);
		array_addByte(hob->ary, ttfTag[ntbl][3]);
		mapTo = table_getMapTable(ttfTbl[ntbl]);

		if (!mapTo)
		{
			array_addLong(hob->ary, ttfTbl[ntbl]->chk);
			array_addLong(hob->ary, ttfTbl[ntbl]->off);
			array_addLong(hob->ary, table_getTableLen(ttfTbl[ntbl]));
			sum += ttfTbl[ntbl]->chk;
		} else
		{
			array_addLong(hob->ary, mapTo->chk);
			array_addLong(hob->ary, mapTo->off);
			array_addLong(hob->ary, table_getTableLen(mapTo));
		}
	}

	sum += table_calcSum(hob);
	table_write(hob, fn);
	for (i = 0; i < numTbl; ++i)
		if (table_getMapTable(ttfTbl[order[i]]) == NULL)
			table_write(ttfTbl[order[i]], fn);
	sum = 0xb1b0afbaUL - sum;
	fseek(fn, ttfTbl[HEAD]->off + 8, SEEK_SET);
	fwrite(&sum, 1, 4, fn);
	
	table_delete(hob);
}

/* -------------------------------------------------------------------------- */

static void map_table_for_macintosh(void)
{
	table_setMapTable(ttfTbl[BLOC], ttfTbl[EBLC]);
	table_setMapTable(ttfTbl[BDAT], ttfTbl[EBDT]);
	table_setMapTable(ttfTbl[BHED], ttfTbl[HEAD]);
}

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

void ttf_output(struct font **fonts, int num_fonts, FILE *fp)
{
	long i;
	struct ttf *ttf;
	struct font *font;

	ttf = (struct ttf *)xmalloc(sizeof(*ttf));
	ttf->flagAutoName = 1;
	ttf->glyphs = (struct glyph *)xmalloc(MAX_GLYPH * sizeof(*ttf->glyphs));
	for (i = 0; i < MAX_GLYPH; i++)
	{
		ttf->glyphs[i].id = -1;
		ttf->glyphs[i].idx = 0;
		ttf->glyphs[i].width = 0;
	}

	ttf->num_glyphs = 0;
	ttf->indexFirst = -1;
	ttf->indexLast = -1;
	for (i = 0; i < num_fonts; i++)
	{
		font = fonts[i];
		for (i = 0; i < MAX_GLYPH; ++i)
		{
			if (is_glyph_available(font, i))
			{
				ttf->glyphs[i].id = ttf->num_glyphs;
				++ttf->num_glyphs;
			}
		}
	}
	
	for (i = 0; i < MAX_GLYPH; ++i)
	{
		if (is_glyph_available(ttf, i))
		{
			if (ttf->indexFirst < 0)
				ttf->indexFirst = (int)i;
			ttf->indexLast = (int)i;
		}
	}

	for (i = 0; i < NUM_TABLE; i++)
		ttfTbl[i] = table_new();

	font = fonts[num_fonts - 1];

	ttf->emX = emCalc(font->max_cell_width, font->form_height);
	ttf->emY = emCalc(font->form_height, font->form_height);
	ttf->emDescent = emCalc(font->descent, font->form_height);
	ttf->emAscent = emCalc(font->top, font->form_height);
	ttf->emDescent = 0;
	ttf->emAscent = ttf->emY;
	
	map_table_for_macintosh();
	generate_EBDT_EBLC_EBSC(ttf, fonts, num_fonts);
	generate_LOCA_GLYF(ttf);
	generate_CMAP(font);
	generate_OS2_PREP(ttf);
	generate_GASP(font);
	generate_HEAD(ttf);
	generate_HHEA(ttf);
	generate_HMTX(font);
	generate_MAXP(font);
	generate_POST(ttf, font);
	generate_NAME(ttf, font);

	write_all_tables(fp);

	for (i = 0; i < NUM_TABLE; i++)
		table_delete(ttfTbl[i]);
}
