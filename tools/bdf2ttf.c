/*
 * bdf2ttf.c - BDF to TTF converter.
 *
 * Written By:	MURAOKA Taro <koron.kaoriya@gmail.com>
 * Last Change: 03-Jul-2016.
 */

/* #define USE_CMAP_3RD_TABLE 1 */

#include "version.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "array.h"
#include "bdf.h"
#include "bdf2ttf.h"
#include "ttnameid.h"

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
#define OS2	6
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
/* Total table number */
#define NUM_TABLE 19

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
	"bloc", "bdat",
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

static const size_t SIZE_BITMAPSIZETABLE = (4 /*sizeof(long) */  * 4
											+ sizeof(char[12]) * 2 + sizeof(short) * 2 + sizeof(char) * 4);
static const size_t SIZE_INDEXSUBTABLEARRAY = 8;
static const int EMMAX = 1024;
static const int MAX_SCALE = 100;

static const char *STYLE_REGULAR = "Regular";
static const char *STYLE_BOLD = "Bold";
static const char *STYLE_ITALIC = "Italic";
static const char *STYLE_BOLDITALIC = "Bold Italic";

static const char *g_style;

const char *g_copyright = NULL;
const char *g_fontname = NULL;
const char *g_version = "1.0";
const char *g_trademark = NULL;

int emCalc(int pix, int base)
{
	return (EMMAX * pix + base / 2) / base;
}

static int order_sort(const void *a, const void *b)
{
	return strcmp(ttfTag[*(int *) a], ttfTag[*(int *) b]);
}

static void addbytes(array *p, const unsigned char *buf, int len)
{
	int i;

	for (i = 0; i < len; ++i)
		array_addByte(p, buf[i]);
}

static void name_addstr(table *t, array *p, const char *buf, short platformID, short encodingID, short languageID, short nameID)
{
	int len;

	if (buf == NULL || (len = strlen(buf)) == 0)
		return;
	array_addShort(t->ary, platformID);
	array_addShort(t->ary, encodingID);
	array_addShort(t->ary, languageID);
	array_addShort(t->ary, nameID);
	array_addShort(t->ary, (short) len);
	array_addShort(t->ary, array_getLen(p));
	addbytes(p, (const unsigned char *) buf, len);
}

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


static void name_addunistr(table *t, array *p, const char *buf, short platformID, short encodingID, short languageID, short nameID)
{
	int unilen;
	unichar_t *unibuf;

	array_addShort(t->ary, platformID);
	array_addShort(t->ary, encodingID);
	array_addShort(t->ary, languageID);
	array_addShort(t->ary, nameID);

	unibuf = into_unicode(&unilen, buf);
	unilen = unilen * sizeof(unichar_t);

	array_addShort(t->ary, (short) unilen);
	array_addShort(t->ary, array_getLen(p));
	array_addWideString(p, unibuf);
	free(unibuf);
}

static int search_topbit(unsigned long n)
{
	int topbit = 0;

	for (; n > 1UL; ++topbit)
		n >>= 1;
	return topbit;
}

static void glyph_simple(bdf2_t *font, array *glyf)
{
	array_addShort(glyf, 1);					/* numberOfContours */
	array_addShort(glyf, 0);					/* xMin */
	array_addShort(glyf, -font->emDescent);		/* yMin */
	array_addShort(glyf, font->emX / 2);		/* xMax */
	/* hmtx define the kerning, not here */
	array_addShort(glyf, font->emAscent);		/* yMax */

	array_addShort(glyf, 0);					/* endPtsOfContours[n] */
	array_addShort(glyf, 0);					/* instructionLength */
	array_addByte(glyf, 0x37);					/* instructions[n] */
	/* On curve, both x,y are positive short */
	array_addByte(glyf, 1);						/* flag[n] */
	array_addByte(glyf, 1);						/* xCoordinates[] */
	array_addShort(glyf, (unsigned short) -1);	/* yCoordinates[] */
	/* for parent add */
}

static unsigned char *get_glyph_bitmap(bdf_glyph_t *glyph)
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
			if (bdf_get_pixel_glyph(glyph, x, y) > 0)
				*p |= mask;
		}
	}
	return buf;
}


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
	array_addLong(st, sizeof(long) * 1 + sizeof(short) * 2);
	/* additionalOffsetToIndexSubtable */
	/* indexSubTable #0 */
	array_addShort(st, 1);					/* indexFormat */
	array_addShort(st, 6);					/* imageFormat */
	array_addLong(st, 0);					/* imageDataOffset */

	array_addLong(st->ary, array_getLen(ebdt->ary));
	for (i = 0; i < BDF_MAX_GLYPH; ++i)
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

static int count_subtable(bdf2_t *font, bdf_t *bdf)
{
	int n_subtbl = 0;
	int i;
	
	for (i = 0; i < BDF_MAX_GLYPH; ++i)
	{
		int idStart;
		int idEnd;
		int encStart;
		int encEnd;
		bdf_boundingbox_t bbx;
		bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
		int j;

		if (!glyph)
			continue;

		memcpy(&bbx, &glyph->bbx, sizeof(bbx));
		idStart = idEnd = font ? bdf2_get_glyph_id(font, i) : glyph->id;
		encStart = encEnd = i;

		for (j = i + 1; j < BDF_MAX_GLYPH; ++j)
		{
			bdf_glyph_t *glyph = bdf_get_glyph(bdf, j);

			if (!glyph)
				continue;
			if (memcmp(&bbx, &glyph->bbx, sizeof(bbx)) != 0)
				break;
			idEnd = font ? bdf2_get_glyph_id(font, j) : glyph->id;
			encEnd = j;
		}
		i = j - 1;
		++n_subtbl;
#if 0
		printf("idStart=%d(%04X), idEnd=%d(%04X)\n", idStart, encStart, idEnd, encEnd);
#endif
		(void) idStart;
		(void) encStart;
	}
	return n_subtbl;
}

static int generate_eb_optbitmap(bdf2_t *font, bdf_t *bdf, table *ebdt, array *st)
{
	int n_subtbl = count_subtable(font, bdf);
	int i;

	/* for indexFormat 2, imageFormat 5 */
	{
		array *sh = array_new();

		for (i = 0; i < BDF_MAX_GLYPH; ++i)
		{
			int idStart;
			int idEnd;
			int encStart;
			int encEnd;
			bdf_boundingbox_t bbx;
			bdf_glyph_t *glyph = bdf_get_glyph(bdf, i);
			int j;
			int w, h, imgsize;

			if (!glyph)
				continue;

			memcpy(&bbx, &glyph->bbx, sizeof(bbx));
			idStart = idEnd = font ? bdf2_get_glyph_id(font, i) : glyph->id;
			encStart = encEnd = i;
			
			for (j = i + 1; j < BDF_MAX_GLYPH; ++j)
			{
				bdf_glyph_t *glyph = bdf_get_glyph(bdf, j);

				if (!glyph)
					continue;
				if (memcmp(&bbx, &glyph->bbx, sizeof(bbx)) != 0)
					break;
				idEnd = font ? bdf2_get_glyph_id(font, j) : glyph->id;
				encEnd = j;
			}
			i = j - 1;

			w = MIN(bbx.width, bdf->bbx.width);
			h = MIN(bbx.height, bdf->bbx.height);
			imgsize = (w * h + 7) >> 3;

			/* indexSubTableArray */
			array_addShort(st, idStart + 1);	/* firstGlyphIndex */
			array_addShort(st, idEnd + 1);	/* lastGlyphIndex */
			array_addLong(st, array_getLen(sh) + SIZE_INDEXSUBTABLEARRAY * n_subtbl);
			/* additionalOffsetToIndexSubtable */

			/* indexSubTable format #2 */
			/* indexSubHeader */
			array_addShort(sh, 2);			/* indexFormat */
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
				glyph = bdf_get_glyph(bdf, j);
				if (!glyph)
				{
					int k;
					
					if (font && bdf2_is_glyph_available(font, j))
						for (k = 0; k < imgsize; ++k)
							array_addByte(ebdt->ary, 0);
				} else
				{
					unsigned char *buf = get_glyph_bitmap(glyph);

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


static void generate_eb_location(bdf2_t *font, bdf_t *bdf,
	table *eblc, array *subtable, array *starray,
	int n_plane, int n_subtbl, int width, int height, int origsize)
{
	int a, d, s;
	int n_glyph;
	
	/* bitmapSizeTable */
	array_addLong(eblc->ary, 4 /*sizeof(long) */  * 2 + SIZE_BITMAPSIZETABLE * n_plane
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
	n_glyph = font ? font->numGlyph : bdf->numGlyph;

	array_addShort(eblc->ary, n_glyph);			/* endGlyphIndex */
	array_addByte(eblc->ary, origsize);			/* ppemX */
	array_addByte(eblc->ary, origsize);			/* ppemY */
	array_addByte(eblc->ary, 1);				/* bitDepth */
	array_addByte(eblc->ary, 0x01);				/* flags: */
	/*  0x01=Horizontal */
	/*  0x02=Vertical */
}

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

static void generate_EBDT_EBLC_EBSC(bdf2_t *bdf2)
{
	int n_plane = bdf2_get_count(bdf2);
	int *sizelist = bdf2_get_sizelist(bdf2);
	int scsize = 1;
	array *starray;
	int i;

	table *ebdt = ttfTbl[EBDT];
	table *eblc = ttfTbl[EBLC];
	table *ebsc = ttfTbl[EBSC];

	array_addLong(ebdt->ary, 0x00020000);
	array_addLong(eblc->ary, 0x00020000);
	array_addLong(eblc->ary, n_plane);
	array_addLong(ebsc->ary, 0x00020000);
	array_addLong(ebsc->ary, MAX_SCALE - n_plane);

	starray = array_new();

	for (i = 0; i < n_plane; ++i)
	{
		int cursize = sizelist[i];
		bdf_t *bdf = bdf2_get_bdf1(bdf2, cursize);
		array *subtable;
		int w, h;
		int orig;
		int n_subtable;
		int nextsize;
		
		if (!bdf)
		{
			fprintf(stderr, "bdf2_t: size %d has not font glyph\n", cursize);
			continue;
		}

		w = bdf->bbx.width;
		h = bdf->bbx.height;

		/*int orig  = MIN(w, h); */
		orig = bdf->pixel_size;		/*(w > h / 2) ? w : w * 2; */
		n_subtable = 0;

		subtable = array_new();
		n_subtable = generate_eb_optbitmap(bdf2, bdf, ebdt, subtable);
		generate_eb_location(bdf2, bdf, eblc, subtable, starray, n_plane, n_subtable, w, h, orig);
		array_addArray(starray, subtable);

		nextsize = (i < n_plane - 1) ? sizelist[i + 1] : MAX_SCALE;

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

static void generate_LOCA_GLYF(bdf2_t *font)
{
	table *loca = ttfTbl[LOCA];
	table *glyf = ttfTbl[GLYF];
	int remain;
	int i;
	
	/* 0 */
	array_addLong(loca->ary, 0);
	glyph_simple(font, glyf->ary);

	remain = font->numGlyph;

	for (i = 0; i < BDF_MAX_GLYPH; ++i)
	{
		if (bdf2_is_glyph_available(font, i))
		{
			array_addLong(loca->ary, array_getLen(glyf->ary));
			--remain;
		}
	}

	table_calcSum(glyf);

	array_addLong(loca->ary, array_getLen(glyf->ary));
	table_calcSum(loca);
}

static void generate_CMAP(bdf2_t *font)
{
	array *fmat0 = array_new();
	array *fmat4 = array_new();
	int numtbl;
	int offset;
	table *cmap;

	/* Make format 0 */
	{
		int i;

		array_addShort(fmat0, 0);				/* format */
		array_addShort(fmat0, sizeof(short) * 3 + 256);
		/* len */
		array_addShort(fmat0, 0);				/* Revision */
		for (i = 0; i < 0x100; ++i)
		{
			int gid = bdf2_is_glyph_available(font, i) ? bdf2_get_glyph_id(font, i) + 1 : 0;

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
		int i, j;
		unsigned short segCountX2;
		unsigned short entrySelector;
		unsigned short searchRange;
		
		for (i = 0; i < BDF_MAX_GLYPH; ++i)
		{
			if (!bdf2_is_glyph_available(font, i))
			{
				if (i == BDF_MAX_GLYPH - 1)
				{
					++segCount;
					array_addShort(endCount, BDF_MAX_GLYPH - 1);
					array_addShort(startCount, BDF_MAX_GLYPH - 1);
					array_addShort(idDeleta, 1);
					array_addShort(idRangeOffset, 0);
				}
				continue;
			}
			j = i + 1;

			while (bdf2_is_glyph_available(font, j))
				++j;
			j -= 1;
			array_addShort(endCount, j);
			array_addShort(startCount, i);
			array_addShort(idDeleta, bdf2_get_glyph_id(font, i) - i + 1);
			array_addShort(idRangeOffset, 0);
			++segCount;
			i = j + 1;
		}

		segCountX2 = segCount * 2;
		entrySelector = search_topbit(segCount);
		searchRange = 1 << (entrySelector + 1);

		array_addShort(fmat4, 4);				/* format */
		array_addShort(fmat4, sizeof(short) * 8 + segCount * sizeof(short) * 4);
		/* length */
		array_addShort(fmat4, 0);				/* language */
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
	offset = sizeof(short) * 2 + (sizeof(short) * 2 + 4 /*sizeof(long) */ ) * numtbl;

	cmap = ttfTbl[CMAP];

	array_addShort(cmap->ary, 0);					/* Version */
	array_addShort(cmap->ary, numtbl);				/* #tbl */
	/* Table 1 (format 0) */
	array_addShort(cmap->ary, 1);					/* Platform ID = 1 -> Macintosh */
	array_addShort(cmap->ary, 0);					/* Encoding ID = 0 -> Roman */
	array_addLong(cmap->ary, offset);
	/* Table 2 (format 4) */
	array_addShort(cmap->ary, 3);					/* Platform ID = 3 -> Microsoft */
	array_addShort(cmap->ary, 1);					/* Encoding ID = 1 -> Unicode */
	array_addLong(cmap->ary, offset + array_getLen(fmat0));
#if USE_CMAP_3RD_TABLE
	/* Table 3 (format X) */
	array_addShort(cmap->ary, 0);					/* Platform ID = X -> Unknown */
	array_addShort(cmap->ary, 0);					/* Encoding ID = X -> Unknown */
	array_addLong(cmap->ary, offset + array_getLen(fmat0));
#endif

	/* Add table main data stream */
	array_addArray(cmap->ary, fmat0);
	array_addArray(cmap->ary, fmat4);

	table_calcSum(cmap);
	
	array_delete(fmat0);
	array_delete(fmat4);
}

static void generate_OS2_PREP(bdf2_t *font)
{
	short fsSelection = 0x0000;
	short usWeightClass = 400;
	char bWeight = 5;
	table *os2;
	
	if (font->flagBold)
	{
		fsSelection |= 0x0020;
		usWeightClass = 700;
		bWeight = 8;
	}
	if (font->flagItalic)
		fsSelection |= 0x0001;
	if (font->flagRegular)
		fsSelection |= 0x0040;

	/* Generate OS/2 and PREP */
	os2 = ttfTbl[OS2];

	array_addShort(os2->ary, 0x0001);				/* version */
	array_addShort(os2->ary, font->emX / 2);		/* xAvgCharWidth */
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
#if 0
	array_addLong(os2->ary, 0xa00002bf);			/* ulUnicodeRange1 */
	array_addLong(os2->ary, 0x68c7fcfb);			/* ulUnicodeRange2 */
	array_addLong(os2->ary, 0x00000000);			/* ulUnicodeRange3 */
	array_addLong(os2->ary, 0x00000000);			/* ulUnicodeRange4 */
#else
	array_addLong(os2->ary, 0xffffffff);			/* ulUnicodeRange1 */
	array_addLong(os2->ary, 0xffffffff);			/* ulUnicodeRange2 */
	array_addLong(os2->ary, 0xffffffff);			/* ulUnicodeRange3 */
	array_addLong(os2->ary, 0xffffffff);			/* ulUnicodeRange4 */
#endif
	array_addString(os2->ary, "KRN ");				/* achVendID */
	array_addShort(os2->ary, fsSelection);			/* fsSelection */
	array_addShort(os2->ary, font->indexFirst);		/* usFirstCharIndex */
	array_addShort(os2->ary, font->indexLast);		/* usLastCharIndex */

	array_addShort(os2->ary, font->emAscent);		/* sTypoAscender */
	array_addShort(os2->ary, -font->emDescent);		/* sTypoDescender */
	array_addShort(os2->ary, 0);					/* sTypoLineGap */
	array_addShort(os2->ary, font->emAscent);		/* usWinAscent */
	array_addShort(os2->ary, font->emDescent);		/* usWinDesent */
#if 1
	/* TODO */
#if 0
	array_addLong(os2->ary, 0x4002000F);			/* ulCodePageRange1 */
	array_addLong(os2->ary, 0xD2000000);			/* ulCodePageRange2 */
#else
	array_addLong(os2->ary, 0xffffffff);			/* ulCodePageRange1 */
	array_addLong(os2->ary, 0xffffffff);			/* ulCodePageRange2 */
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

	addbytes(ttfTbl[PREP]->ary, &prep_prog[0], arraysize(prep_prog));
	table_calcSum(ttfTbl[PREP]);
}


static void generate_GASP(bdf2_t *font)
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

static void generate_HEAD_HHEA_HMTX_MAXP_POST(bdf2_t *font)
{
	/*gene head, hhea, hmtx, maxp, and post */
	short macStyle = 0x0000;
	table *tmp;
	int i;

	if (font->flagBold)
		macStyle |= 0x0001;
	if (font->flagItalic)
		macStyle |= 0x0002;

	tmp = ttfTbl[HEAD];

	array_addLong(tmp->ary, 0x10000);				/* version */
	array_addLong(tmp->ary, 0x10000);				/* fontRevision */
	array_addLong(tmp->ary, 0);						/* checkSumAdjustment */
	array_addLong(tmp->ary, 0x5f0f3cf5);			/* magicNumber */
	array_addShort(tmp->ary, 0x000b);				/* flags */
	array_addShort(tmp->ary, EMMAX);				/* unitPerEm */
	array_addLong(tmp->ary, 0);						/* createdTime */
	array_addLong(tmp->ary, 0);
	array_addLong(tmp->ary, 0);						/* modifiedTime */
	array_addLong(tmp->ary, 0);
	array_addShort(tmp->ary, 0);					/* xMin */
	array_addShort(tmp->ary, -font->emDescent);		/* yMin */
	array_addShort(tmp->ary, font->emX);			/* xMax */
	array_addShort(tmp->ary, font->emAscent);		/* yMax */
	array_addShort(tmp->ary, macStyle);				/* macStyle */
	array_addShort(tmp->ary, 7);					/* lowestRecPPEM */
	array_addShort(tmp->ary, 1);					/* fontDirectionHint */
	array_addShort(tmp->ary, 1);					/* indexToLocFormat */
	array_addShort(tmp->ary, 0);					/* glyphDataFormat */
	table_calcSum(tmp);

	tmp = ttfTbl[HHEA];
	array_addLong(tmp->ary, 0x10000);				/* version */
	array_addShort(tmp->ary, font->emAscent);		/* Ascender */
	array_addShort(tmp->ary, -font->emDescent);		/* Descender */
	array_addShort(tmp->ary, 0);					/* yLineGap */
	array_addShort(tmp->ary, font->emX);			/* advanceWidthMax */
	array_addShort(tmp->ary, 0);					/* minLeftSideBearing */
	array_addShort(tmp->ary, 0);					/* minRightSideBearing */
	array_addShort(tmp->ary, font->emX);			/* xMaxExtent */
	array_addShort(tmp->ary, 1);					/* caretSlopeRise */
	array_addShort(tmp->ary, 0);					/* caretSlopeRun */
	array_addShort(tmp->ary, 0);					/* caretOffset */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* (reserved) */
	array_addShort(tmp->ary, 0);					/* metricDataFormat */
	array_addShort(tmp->ary, font->numGlyph + 1);	/* numberOfHMetrics */
	table_calcSum(tmp);

	tmp = ttfTbl[HMTX];
	/* #0 */
	array_addShort(tmp->ary, emCalc(1, 2));
	array_addShort(tmp->ary, 0);
	for (i = 0; i < BDF_MAX_GLYPH; ++i)
	{
		if (!bdf2_is_glyph_available(font, i))
			continue;
		array_addShort(tmp->ary, emCalc(bdf2_get_glyph_width(font, i), 2));
		array_addShort(tmp->ary, 0);
	}
	table_calcSum(tmp);

	tmp = ttfTbl[MAXP];
	array_addLong(tmp->ary, 0x10000);				/* Version */
	array_addShort(tmp->ary, font->numGlyph + 1);	/* numGlyphs */
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

	tmp = ttfTbl[POST];
	array_addLong(tmp->ary, 0x30000);				/* Version */
	array_addLong(tmp->ary, 0);						/* italicAngle */
	array_addShort(tmp->ary, -font->emDescent);		/* underlinePosition */
#if 0
	array_addShort(tmp->ary, emCalc(1, g_bdf->bbx.width));
#else
	array_addShort(tmp->ary, 1);					/* underlineThickness */
#endif
	/* underlineThickness */
	array_addLong(tmp->ary, 1);						/* isFixedPitch */
	array_addLong(tmp->ary, 0);						/* minMemType42 */
	array_addLong(tmp->ary, 0);						/* maxMemType42 */
	array_addLong(tmp->ary, 0);						/* minMemType1 */
	array_addLong(tmp->ary, 0);						/* maxMemType1 */
	table_calcSum(tmp);
}


static void generate_NAME(bdf2_t *font)
{
	static const int NUM = 16;
	array *p = array_new();
	table *t = ttfTbl[NAME];
	char fullname[1024];

	g_style = STYLE_REGULAR;
	if (font->flagAutoName)
	{
		if (font->flagBold && font->flagItalic)
			g_style = STYLE_BOLDITALIC;
		else if (font->flagBold)
			g_style = STYLE_BOLD;
		else if (font->flagItalic)
			g_style = STYLE_ITALIC;
	}

	strcpy(fullname, g_fontname);
	if (g_style != STYLE_REGULAR)
	{
		strcat(fullname, " ");
		strcat(fullname, g_style);
	}

	array_addShort(t->ary, 0);						/* format selector */
	array_addShort(t->ary, NUM);					/* #NameRec */
	array_addShort(t->ary, 6 + NUM * 12);			/* Offset of strings */

	name_addstr(t, p, g_copyright, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_COPYRIGHT);
	name_addstr(t, p, g_fontname, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FONT_FAMILY);
	name_addstr(t, p, g_style, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FONT_SUBFAMILY);
	name_addstr(t, p, g_fontname, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_UNIQUE_ID);
	name_addstr(t, p, fullname, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_FULL_NAME);
	name_addstr(t, p, g_version, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_VERSION_STRING);
	name_addstr(t, p, g_fontname, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_PS_NAME);
	name_addstr(t, p, g_trademark, TT_PLATFORM_MACINTOSH, TT_APPLE_ID_DEFAULT, TT_MAC_LANGID_ENGLISH, TT_NAME_ID_TRADEMARK);

	name_addunistr(t, p, g_copyright, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_COPYRIGHT);
	name_addunistr(t, p, g_fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FONT_FAMILY);
	name_addunistr(t, p, g_style, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FONT_SUBFAMILY);
	name_addunistr(t, p, g_fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_UNIQUE_ID);
	name_addunistr(t, p, fullname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_FULL_NAME);
	name_addunistr(t, p, g_version, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_VERSION_STRING);
	name_addunistr(t, p, g_fontname, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_PS_NAME);
	name_addunistr(t, p, g_trademark, TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, TT_MS_LANGID_ENGLISH_UNITED_STATES, TT_NAME_ID_TRADEMARK);

	array_addArray(t->ary, p);
	table_calcSum(t);
	
	array_delete(p);
}

static void write_all_tables(bdf2_t *font, FILE *fn)
{
	int i;

	/* tableOrder */
	static const int order[] = {
		OS2, CMAP, HEAD, HHEA, HMTX, MAXP, NAME, POST,
		EBLC, EBDT, EBSC,
		GASP,
		/* CVT, FPGM, */
		PREP, LOCA, GLYF,
		BLOC, BDAT,
	};
	table *hob = table_new();
	unsigned long sum = 0;
	int numTbl = arraysize(order);
	int order2[arraysize(order)];

	(void)font;

	array_addLong(hob->ary, 0x10000);				/* Version */
	array_addShort(hob->ary, numTbl);
	array_addShort(hob->ary, 128);
	array_addShort(hob->ary, 3);
	array_addShort(hob->ary, numTbl * 16 - 128);

	ttfTbl[order[0]]->off = 12 + numTbl * 16;
	for (i = 1; i < numTbl; ++i)
	{
		int prev = order[i - 1];

		ttfTbl[order[i]]->off = ttfTbl[prev]->off + table_getTableLen(ttfTbl[prev]);
	}

	memcpy(order2, order, sizeof(order));
	qsort(order2, numTbl, sizeof(int), order_sort);
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
	sum = 0xb1b0afba - sum;
	fseek(fn, ttfTbl[HEAD]->off + 8, SEEK_SET);
	fwrite(&sum, 1, 4, fn);
	
	table_delete(hob);
}


static void map_table_for_macintosh(bdf2_t *font)
{
	(void)font;

	table_setMapTable(ttfTbl[BLOC], ttfTbl[EBLC]);
	table_setMapTable(ttfTbl[BDAT], ttfTbl[EBDT]);
}


int write_ttf(bdf2_t *font, const char *ttfname)
{
	FILE *fp = fopen(ttfname, "wb+");
	int i;
	
	if (fp == NULL)
	{
		fprintf(stderr, "write_ttf:ERROR: Can't open a file\n");
		return 0;
	}

	for (i = 0; i < NUM_TABLE; i++)
		ttfTbl[i] = table_new();

	map_table_for_macintosh(font);
	generate_EBDT_EBLC_EBSC(font);
	generate_LOCA_GLYF(font);
	generate_CMAP(font);
	generate_OS2_PREP(font);
	generate_GASP(font);
	generate_HEAD_HHEA_HMTX_MAXP_POST(font);
	generate_NAME(font);

	write_all_tables(font, fp);
	fclose(fp);

	for (i = 0; i < NUM_TABLE; i++)
		table_delete(ttfTbl[i]);

	return 1;
}
