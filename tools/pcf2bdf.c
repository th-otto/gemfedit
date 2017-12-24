/* pcf2bdf.c */

/*
 * see libXfont-1.4.5: src/bitmap/pcfread.c, pcfwrite.c, bcfread.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(_MSC_VER)
#  include <io.h>
#  include <fcntl.h>
#  include <process.h>
#  define popen _popen
#elif defined(__CYGWIN__)
#  include <io.h>
#  include <sys/fcntl.h>
#else
#  define _setmode(fd, mode)
#endif


/* miscellaneous definition */


/* section ID */
enum type32
{
	PCF_PROPERTIES = (1 << 0),
	PCF_ACCELERATORS = (1 << 1),
	PCF_METRICS = (1 << 2),
	PCF_BITMAPS = (1 << 3),
	PCF_INK_METRICS = (1 << 4),
	PCF_BDF_ENCODINGS = (1 << 5),
	PCF_SWIDTHS = (1 << 6),
	PCF_GLYPH_NAMES = (1 << 7),
	PCF_BDF_ACCELERATORS = (1 << 8),
};

/* section format */
struct format32
{
	uint32_t id:24;						/* one of four constants below */
	uint32_t dummy:2;						/* = 0 padding */
	uint32_t scan:2;						/* read bitmap by (1 << scan) bytes */
	uint32_t bit:1;						/* 0:LSBit first, 1:MSBit first */
	uint32_t byte:1;						/* 0:LSByte first, 1:MSByte first */
	uint32_t glyph:2;						/* a scanline of gryph is aligned by (1 << glyph) bytes */
};

static bool is_little_endian(const struct format32 *format)
{
	return !format->byte;
}

/* format32.id is: */
#define PCF_DEFAULT_FORMAT     0
#define PCF_INKBOUNDS          2
#define PCF_ACCEL_W_INKBOUNDS  1
#define PCF_COMPRESSED_METRICS 1
/* BDF file is outputed: MSBit first and MSByte first */
const struct format32 BDF_format = { PCF_DEFAULT_FORMAT, 0, 0, 1, 1, 0 };

/* string or value */
union sv
{
	char *s;
	int32_t v;
};

/* metric informations */
struct metric_t
{
	int16_t leftSideBearing;			/* leftmost coordinate of the gryph */
	int16_t rightSideBearing;			/* rightmost coordinate of the gryph */
	int16_t characterWidth;				/* offset to next gryph */
	int16_t ascent;						/* pixels below baseline */
	int16_t descent;					/* pixels above Baseline */
	uint16_t attributes;

	uint8_t *bitmaps;					/* bitmap pattern of gryph */
	int32_t swidth;						/* swidth */
	union sv glyphName;					/* name of gryph */

};

/* glyph width */
static int16_t widthBits(const struct metric_t *m)
{
	return m->rightSideBearing - m->leftSideBearing;
}


/* glyph height */
static int16_t height(const struct metric_t *m)
{
	return m->ascent + m->descent;
}


static int16_t bytesPerRow(int bits, int nbytes)
{
	return nbytes == 1 ? ((bits + 7) >> 3)	/* pad to 1 byte */
		: nbytes == 2 ? (((bits + 15) >> 3) & ~1)	/* pad to 2 bytes */
		: nbytes == 4 ? (((bits + 31) >> 3) & ~3)	/* pad to 4 bytes */
		: nbytes == 8 ? (((bits + 63) >> 3) & ~7)	/* pad to 8 bytes */
		: 0;
}

/* bytes for one scanline */
static int16_t widthBytes(const struct metric_t *m, const struct format32 *f)
{
	return bytesPerRow(widthBits(m), 1 << f->glyph);
}

#define GLYPHPADOPTIONS 4

#define make_charcode(row,col) (row * 256 + col)
#define NO_SUCH_CHAR 0xffff


/* global variables */


/* table of contents */
static int32_t nTables;

static struct table_t
{
	enum type32 type;					/* section ID */
	struct format32 format;				/* section format */
	int32_t size;						/* size of section */
	int32_t offset;						/* byte offset from the beginning of the file */
} *tables;

/* properties section */
static int32_t nProps;					/* number of properties */

static struct props_t
{										/* property */
	union sv name;						/* name of property */
	bool isStringProp;					/* whether this property is a string (or a value) */
	union sv value;						/* the value of this property */
} *props;

static int32_t stringSize;				/* size of string */

static char *string;					/* string used in property */

/* accelerators section */
static struct accelerators_t
{
	bool noOverlap;						/* true if:
										 * max(rightSideBearing - characterWidth) <=
										 * minbounds->metrics.leftSideBearing */
	bool constantMetrics;
	bool terminalFont;					/* true if:
										 * constantMetrics && leftSideBearing == 0 &&
										 * rightSideBearing == characterWidth &&
										 * ascent == fontAscent &&
										 * descent == fontDescent */
	bool constantWidth;					/* true if:
										 * minbounds->metrics.characterWidth
										 * ==
										 * maxbounds->metrics.characterWidth */
	bool inkInside;						/* true if for all defined glyphs:
										 * 0 <= leftSideBearing &&
										 * rightSideBearing <= characterWidth &&
										 * -fontDescent <= ascent <= fontAscent &&
										 * -fontAscent <= descent <= fontDescent */
	bool inkMetrics;					/* ink metrics != bitmap metrics */
	bool drawDirection;					/* 0:L->R 1:R->L */
	int32_t fontAscent;
	int32_t fontDescent;
	int32_t maxOverlap;
	struct metric_t minBounds;
	struct metric_t maxBounds;
	struct metric_t ink_minBounds;
	struct metric_t ink_maxBounds;
} accelerators;

/* metrics section */
static int32_t nMetrics;
static struct metric_t *metrics;

/* bitmaps section */
static int32_t nBitmaps;
static uint32_t *bitmapOffsets;
static uint32_t bitmapSizes[GLYPHPADOPTIONS];
static uint8_t *bitmaps;					/* bitmap patterns of the gryph */
static int32_t bitmapSize;					/* size of bitmaps */

/* encodings section */
static uint16_t firstCol;
static uint16_t lastCol;
static uint16_t firstRow;
static uint16_t lastRow;
static uint16_t defaultCh;					/* default character */
static uint16_t *encodings;
static int nEncodings;						/* number of encodings */
static int nValidEncodings;					/* number of valid encodings */

/* swidths section */
static int32_t nSwidths;

/* glyph names section */
static int32_t nGlyphNames;
static int32_t glyphNamesSize;
static char *glyphNames;

/* other globals */
static FILE *ifp;							/* input file pointer */
static FILE *ofp;							/* output file pointer */
static long read_bytes;						/* read bytes */
static struct format32 format;				/* current section format */
static struct metric_t fontbbx;				/* font bounding box */
static bool verbose;						/* show messages verbosely */


/* miscellaneous functions */


static int error_exit(const char *str)
{
	fprintf(stderr, "pcf2bdf: %s\n", str);
	exit(1);
	return 1;
}

static int error_invalid_exit(const char *str)
{
	fprintf(stderr, "pcf2bdf: <%s> invalid PCF file\n", str);
	exit(1);
	return 1;
}

static int check_memory(void *ptr)
{
	if (!ptr)
	{
		return error_exit("out of memory");
	}
	return 0;
}


static uint8_t *read_byte8s(uint8_t *mem, size_t size)
{
	size_t read_size = fread(mem, 1, size, ifp);

	if (read_size != size)
	{
		error_exit("unexpected eof");
	}
	read_bytes += size;
	return mem;
}


static char read8(void)
{
	int a = fgetc(ifp);

	read_bytes++;
	if (a == EOF)
	{
		return (char) error_exit("unexpected eof");
	}
	return (char) a;
}

static bool read_bool(void)
{
	return read8() != 0;
}

static uint8_t read_uint8(void)
{
	return (uint8_t) read8();
}


/* These all return int rather than int16_t in order to handle values
 * between 32768 and 65535 more gracefully.
 */
static int make_int16(int a, int b)
{
	int value;

	value = (a & 0xff) << 8;
	value |= (b & 0xff);
	return value;
}

static int read_int16_big(void)
{
	int a = read8();
	int b = read8();

	return make_int16(a, b);
}

static int read_int16_little(void)
{
	int a = read8();
	int b = read8();

	return make_int16(b, a);
}

static int read_int16(void)
{
	if (is_little_endian(&format))
	{
		return read_int16_little();
	} else
	{
		return read_int16_big();
	}
}


static int32_t make_int32(int a, int b, int c, int d)
{
	int32_t value;

	value = (int32_t) (a & 0xff) << 24;
	value |= (int32_t) (b & 0xff) << 16;
	value |= (int32_t) (c & 0xff) << 8;
	value |= (int32_t) (d & 0xff);
	return value;
}

static int32_t read_int32_big(void)
{
	int a = read8();
	int b = read8();
	int c = read8();
	int d = read8();

	return make_int32(a, b, c, d);
}

static int32_t read_int32_little(void)
{
	int a = read8();
	int b = read8();
	int c = read8();
	int d = read8();

	return make_int32(d, c, b, a);
}

static int32_t read_int32(void)
{
	if (is_little_endian(&format))
	{
		return read_int32_little();
	} else
	{
		return read_int32_big();
	}
}

static uint32_t read_uint32(void)
{
	return (uint32_t) read_int32();
}

static struct format32 read_format32_little(void)
{
	int32_t v = read_int32_little();
	struct format32 f;

	f.id = v >> 8;
	f.dummy = 0;
	f.scan = v >> 4;
	f.bit = v >> 3;
	f.byte = v >> 2;
	f.glyph = v >> 0;
	return f;
}


static void skip(int n)
{
	for (; 0 < n; n--)
	{
		read8();
	}
}


static void bit_order_invert(uint8_t *data, int size)
{
	static const uint8_t invert[16] = { 0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15 };
	for (int i = 0; i < size; i++)
	{
		data[i] = (invert[data[i] & 15] << 4) | invert[(data[i] >> 4) & 15];
	}
}

static void two_byte_swap(uint8_t *data, int size)
{
	size &= ~1;
	for (int i = 0; i < size; i += 2)
	{
		uint8_t tmp = data[i];

		data[i] = data[i + 1];
		data[i + 1] = tmp;
	}
}

static void four_byte_swap(uint8_t *data, int size)
{
	size &= ~3;
	for (int i = 0; i < size; i += 4)
	{
		uint8_t tmp = data[i];

		data[i] = data[i + 3];
		data[i + 3] = tmp;
		tmp = data[i + 1];
		data[i + 1] = data[i + 2];
		data[i + 2] = tmp;
	}
}


/* main */


/* search and seek a section of 'type' */
static bool seek(enum type32 type)
{
	for (int i = 0; i < nTables; i++)
	{
		if (tables[i].type == type)
		{
			int s = tables[i].offset - read_bytes;

			if (s < 0)
			{
				error_invalid_exit("seek");
			}
			skip(s);
			return true;
		}
	}
	return false;
}


/* does a section of 'type' exist? */
static bool is_exist_section(enum type32 type)
{
	for (int i = 0; i < nTables; i++)
	{
		if (tables[i].type == type)
		{
			return true;
		}
	}
	return false;
}


/* read metric information */
static void read_metric(struct metric_t *m)
{
	m->leftSideBearing = read_int16();
	m->rightSideBearing = read_int16();
	m->characterWidth = read_int16();
	m->ascent = read_int16();
	m->descent = read_int16();
	m->attributes = read_int16();
}


/* read compressed metric information */
static void read_compressed_metric(struct metric_t *m)
{
	m->leftSideBearing = (int16_t) read_uint8() - 0x80;
	m->rightSideBearing = (int16_t) read_uint8() - 0x80;
	m->characterWidth = (int16_t) read_uint8() - 0x80;
	m->ascent = (int16_t) read_uint8() - 0x80;
	m->descent = (int16_t) read_uint8() - 0x80;
	m->attributes = 0;
}


static void verbose_metric(struct metric_t *m, const char *name)
{
	if (verbose)
	{
		fprintf(stderr, "\t%s.leftSideBearing  = %d\n", name, m->leftSideBearing);
		fprintf(stderr, "\t%s.rightSideBearing = %d\n", name, m->rightSideBearing);
		fprintf(stderr, "\t%s.characterWidth   = %d\n", name, m->characterWidth);
		fprintf(stderr, "\t%s.ascent           = %d\n", name, m->ascent);
		fprintf(stderr, "\t%s.descent          = %d\n", name, m->descent);
		fprintf(stderr, "\t%s.attributes       = %04x\n", name, m->attributes);
	}
}


/* read accelerators section */
static void read_accelerators(void)
{
	format = read_format32_little();
	if (!(format.id == PCF_DEFAULT_FORMAT || format.id == PCF_ACCEL_W_INKBOUNDS))
	{
		error_invalid_exit("accelerators");
	}

	accelerators.noOverlap = read_bool();
	accelerators.constantMetrics = read_bool();
	accelerators.terminalFont = read_bool();
	accelerators.constantWidth = read_bool();
	accelerators.inkInside = read_bool();
	accelerators.inkMetrics = read_bool();
	accelerators.drawDirection = read_bool();
	/* dummy */ read_bool();
	accelerators.fontAscent = read_int32();
	accelerators.fontDescent = read_int32();
	accelerators.maxOverlap = read_int32();
	if (verbose)
	{
		fprintf(stderr, "\tnoOverlap       = %d\n", (int) accelerators.noOverlap);
		fprintf(stderr, "\tconstantMetrics = %d\n", (int) accelerators.constantMetrics);
		fprintf(stderr, "\tterminalFont    = %d\n", (int) accelerators.terminalFont);
		fprintf(stderr, "\tconstantWidth   = %d\n", (int) accelerators.constantWidth);
		fprintf(stderr, "\tinkInside       = %d\n", (int) accelerators.inkInside);
		fprintf(stderr, "\tinkMetrics      = %d\n", (int) accelerators.inkMetrics);
		fprintf(stderr, "\tdrawDirection   = %d\n", (int) accelerators.drawDirection);
		fprintf(stderr, "\tfontAscent      = %d\n", (int) accelerators.fontAscent);
		fprintf(stderr, "\tfontDescent     = %d\n", (int) accelerators.fontDescent);
		fprintf(stderr, "\tmaxOverlap      = %d\n", (int) accelerators.maxOverlap);
	}
	read_metric(&accelerators.minBounds);
	read_metric(&accelerators.maxBounds);
	verbose_metric(&accelerators.minBounds, "minBounds");
	verbose_metric(&accelerators.maxBounds, "maxBounds");
	if (format.id == PCF_ACCEL_W_INKBOUNDS)
	{
		read_metric(&accelerators.ink_minBounds);
		read_metric(&accelerators.ink_maxBounds);
		verbose_metric(&accelerators.ink_minBounds, "ink_minBounds");
		verbose_metric(&accelerators.ink_maxBounds, "ink_maxBounds");
	} else
	{
		accelerators.ink_minBounds = accelerators.minBounds;
		accelerators.ink_maxBounds = accelerators.maxBounds;
	}
}


/* search a property named 'name', and return its string if it is a string */
static char *get_property_string(const char *name)
{
	for (int i = 0; i < nProps; i++)
	{
		if (strcmp(name, props[i].name.s) == 0)
		{
			if (props[i].isStringProp)
			{
				return props[i].value.s;
			} else
			{
				error_invalid_exit("property_string");
			}
		}
	}
	return NULL;
}


/* search a property named 'name', and return its value if it is a value */
static int32_t get_property_value(const char *name)
{
	for (int i = 0; i < nProps; i++)
	{
		if (strcmp(name, props[i].name.s) == 0)
		{
			if (props[i].isStringProp)
			{
				error_invalid_exit("property_value");
			} else
			{
				return props[i].value.v;
			}
		}
	}
	return -1;
}


/* does a property named 'name' exist? */
static bool is_exist_property_value(const char *name)
{
	for (int i = 0; i < nProps; i++)
	{
		if (strcmp(name, props[i].name.s) == 0)
		{
			if (props[i].isStringProp)
			{
				return false;
			} else
			{
				return true;
			}
		}
	}
	return false;
}


static int usage_exit(void)
{
	printf("usage: pcf2bdf [-v] [-o bdf file] [pcf file]\n");
	return 1;
}


int main(int argc, char *argv[])
{
	int i;
	char *ifilename = NULL;
	char *ofilename = NULL;

	/* read options */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (argv[i][1] == 'v')
			{
				verbose = true;
			} else if (i + 1 == argc || argv[i][1] != 'o' || ofilename)
			{
				return usage_exit();
			} else
			{
				ofilename = argv[++i];
			}
		} else
		{
			if (ifilename)
			{
				return usage_exit();
			} else
			{
				ifilename = argv[i];
			}
		}
	}
	if (ifilename)
	{
		ifp = fopen(ifilename, "rb");
		if (!ifp)
		{
			return error_exit("failed to open input pcf file");
		}
	} else
	{
		_setmode(fileno(stdin), O_BINARY);
		ifp = stdin;
	}
	int32_t version = read_int32_big();

	if ((version >> 16) == 0x1f9d ||	/* compress'ed */
		(version >> 16) == 0x1f8b)		/* gzip'ed */
	{
		if (!ifilename)
		{
			return error_exit("stdin is gzip'ed or compress'ed\n");
		}
		fclose(ifp);
		char buf[1024];

		sprintf(buf, "gzip -dc %s", ifilename);	/* TODO */
		ifp = popen(buf, "r");
		_setmode(fileno(ifp), O_BINARY);
		read_bytes = 0;
		if (!ifp)
		{
			return error_exit("failed to execute gzip\n");
		}
	}

	if (ofilename)
	{
		ofp = fopen(ofilename, "wb");
		if (!ofp)
		{
			return error_exit("failed to open output bdf file");
		}
	} else
	{
		ofp = stdout;
	}

	/* read PCF file */

	/* read table of contents */
	if (read_bytes == 0)
	{
		version = read_int32_big();
	}
	if (version != make_int32(1, 'f', 'c', 'p'))
	{
		error_exit("this is not PCF file format");
	}
	nTables = read_int32_little();
	tables = (struct table_t *)malloc(nTables * sizeof(*tables));
	check_memory(tables);
	for (i = 0; i < nTables; i++)
	{
		tables[i].type = (enum type32) read_int32_little();
		tables[i].format = read_format32_little();
		tables[i].size = read_int32_little();
		tables[i].offset = read_int32_little();
	}

	/* read properties section */
	if (!seek(PCF_PROPERTIES))
	{
		error_exit("PCF_PROPERTIES does not found");
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_PROPERTIES\n");
		}
	}
	format = read_format32_little();
	if (!(format.id == PCF_DEFAULT_FORMAT))
	{
		error_invalid_exit("properties(format)");
	}
	nProps = read_int32();
	props = (struct props_t *)malloc(nProps * sizeof(*props));
	check_memory(props);
	for (i = 0; i < nProps; i++)
	{
		props[i].name.v = read_int32();
		props[i].isStringProp = read_bool();
		props[i].value.v = read_int32();
	}
	skip(3 - (((4 + 1 + 4) * nProps + 3) % 4));
	stringSize = read_int32();
	string = (char *)malloc((stringSize + 1) * sizeof(*string));
	check_memory(string);
	read_byte8s((uint8_t *) string, stringSize);
	string[stringSize] = '\0';
	for (i = 0; i < nProps; i++)
	{
		if (stringSize <= props[i].name.v)
		{
			error_invalid_exit("properties(name)");
		}
		props[i].name.s = string + props[i].name.v;
		if (verbose)
		{
			fprintf(stderr, "\t%s ", props[i].name.s);
		}
		if (props[i].isStringProp)
		{
			if (stringSize <= props[i].value.v)
			{
				error_invalid_exit("properties(value)");
			}
			props[i].value.s = string + props[i].value.v;
			if (verbose)
			{
				fprintf(stderr, "\"%s\"\n", props[i].value.s);
			}
		} else
		{
			if (verbose)
			{
				fprintf(stderr, "%d\n", props[i].value.v);
			}
		}
	}

	/* read old accelerators section */
	if (!is_exist_section(PCF_BDF_ACCELERATORS))
	{
		if (!seek(PCF_ACCELERATORS))
		{
			error_exit("PCF_ACCELERATORS and PCF_BDF_ACCELERATORS do not found");
		} else
		{
			if (verbose)
			{
				fprintf(stderr, "PCF_ACCELERATORS\n");
			}
			read_accelerators();
		}
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "(PCF_ACCELERATORS)\n");
		}
	}

	/* read metrics section */
	if (!seek(PCF_METRICS))
	{
		error_exit("PCF_METRICS does not found");
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_METRICS\n");
		}
	}
	format = read_format32_little();
	switch (format.id)
	{
	default:
		error_invalid_exit("metrics");
	case PCF_DEFAULT_FORMAT:
		nMetrics = read_int32();
		metrics = (struct metric_t *)calloc(nMetrics, sizeof(struct metric_t));
		check_memory(metrics);
		for (i = 0; i < nMetrics; i++)
		{
			read_metric(&metrics[i]);
		}
		break;
	case PCF_COMPRESSED_METRICS:
		if (verbose)
		{
			fprintf(stderr, "\tPCF_COMPRESSED_METRICS\n");
		}
		nMetrics = read_int16();
		metrics = (struct metric_t *)calloc(nMetrics, sizeof(struct metric_t));
		check_memory(metrics);
		for (i = 0; i < nMetrics; i++)
		{
			read_compressed_metric(&metrics[i]);
		}
		break;
	}
	if (verbose)
	{
		fprintf(stderr, "\tnMetrics = %d\n", nMetrics);
	}
	fontbbx = metrics[0];
	for (i = 1; i < nMetrics; i++)
	{
		if (metrics[i].leftSideBearing < fontbbx.leftSideBearing)
		{
			fontbbx.leftSideBearing = metrics[i].leftSideBearing;
		}
		if (fontbbx.rightSideBearing < metrics[i].rightSideBearing)
		{
			fontbbx.rightSideBearing = metrics[i].rightSideBearing;
		}
		if (fontbbx.ascent < metrics[i].ascent)
		{
			fontbbx.ascent = metrics[i].ascent;
		}
		if (fontbbx.descent < metrics[i].descent)
		{
			fontbbx.descent = metrics[i].descent;
		}
	}

	/* read bitmaps section */
	if (!seek(PCF_BITMAPS))
	{
		error_exit("PCF_BITMAPS does not found");
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_BITMAPS\n");
		}
	}
	format = read_format32_little();
	if (!(format.id == PCF_DEFAULT_FORMAT))
	{
		error_invalid_exit("bitmaps");
	}
	nBitmaps = read_int32();
	bitmapOffsets = (uint32_t *)malloc(nBitmaps * sizeof(*bitmapOffsets));
	check_memory(bitmapOffsets);
	for (i = 0; i < nBitmaps; i++)
	{
		bitmapOffsets[i] = read_uint32();
	}
	for (i = 0; i < GLYPHPADOPTIONS; i++)
	{
		bitmapSizes[i] = read_uint32();
	}
	bitmapSize = bitmapSizes[format.glyph];
	bitmaps = (uint8_t *)malloc(bitmapSize * sizeof(*bitmaps));
	check_memory(bitmaps);
	read_byte8s(bitmaps, bitmapSize);

	if (verbose)
	{
		fprintf(stderr, "\t1<<format.scan = %d\n", 1 << format.scan);
		fprintf(stderr, "\t%sSBit first\n", format.bit ? "M" : "L");
		fprintf(stderr, "\t%sSByte first\n", format.byte ? "M" : "L");
		fprintf(stderr, "\t1<<format.glyph = %d\n", 1 << format.glyph);
	}
	if (format.bit != BDF_format.bit)
	{
		if (verbose)
		{
			fprintf(stderr, "\tbit_order_invert()\n");
		}
		bit_order_invert(bitmaps, bitmapSize);
	}
	if ((format.bit == format.byte) != (BDF_format.bit == BDF_format.byte))
	{
		switch (1 << (BDF_format.bit == BDF_format.byte ? format.scan : BDF_format.scan))
		{
		case 1:
			break;
		case 2:
			if (verbose)
			{
				fprintf(stderr, "\ttwo_byte_swap()\n");
			}
			two_byte_swap(bitmaps, bitmapSize);
			break;
		case 4:
			if (verbose)
			{
				fprintf(stderr, "\tfour_byte_swap()\n");
			}
			four_byte_swap(bitmaps, bitmapSize);
			break;
		}
	}

	for (i = 0; i < nMetrics; i++)
	{
		struct metric_t *m = &metrics[i];
		m->bitmaps = bitmaps + bitmapOffsets[i];
	}

	/* ink metrics section is ignored */

	/* read encodings section */
	if (!seek(PCF_BDF_ENCODINGS))
	{
		error_exit("PCF_BDF_ENCODINGS does not found");
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_ENCODINGS\n");
		}
	}
	format = read_format32_little();
	if (!(format.id == PCF_DEFAULT_FORMAT))
	{
		error_invalid_exit("encoding");
	}
	firstCol = read_int16();
	lastCol = read_int16();
	firstRow = read_int16();
	lastRow = read_int16();
	defaultCh = read_int16();
	if (verbose)
	{
		fprintf(stderr, "\tfirstCol  = %X\n", firstCol);
		fprintf(stderr, "\tlastCol   = %X\n", lastCol);
		fprintf(stderr, "\tfirstRow  = %X\n", firstRow);
		fprintf(stderr, "\tlastRow   = %X\n", lastRow);
		fprintf(stderr, "\tdefaultCh = %X\n", defaultCh);
	}
	nEncodings = (lastCol - firstCol + 1) * (lastRow - firstRow + 1);
	encodings = (uint16_t *)malloc(nEncodings * sizeof(*encodings));
	check_memory(encodings);
	for (i = 0; i < nEncodings; i++)
	{
		encodings[i] = read_int16();
		if (encodings[i] != NO_SUCH_CHAR)
		{
			nValidEncodings++;
		}
	}

	/* read swidths section */
	if (seek(PCF_SWIDTHS))
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_SWIDTHS\n");
		}
		format = read_format32_little();
		if (!(format.id == PCF_DEFAULT_FORMAT))
		{
			error_invalid_exit("encoding");
		}
		nSwidths = read_int32();
		if (nSwidths != nMetrics)
		{
			error_exit("nSwidths != nMetrics");
		}
		for (i = 0; i < nSwidths; i++)
		{
			metrics[i].swidth = read_int32();
		}
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "no PCF_SWIDTHS\n");
		}
		int32_t rx = get_property_value("RESOLUTION_X");

		if (rx <= 0)
		{
			rx = (int) (get_property_value("RESOLUTION") / 100.0 * 72.27);
		}
		double p = get_property_value("POINT_SIZE") / 10.0;

		for (i = 0; i < nSwidths; i++)
		{
			metrics[i].swidth = (int) (metrics[i].characterWidth / (rx / 72.27) / (p / 1000));
		}
	}

	/* read glyph names section */
	if (seek(PCF_GLYPH_NAMES))
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_GLYPH_NAMES\n");
		}
		format = read_format32_little();
		if (!(format.id == PCF_DEFAULT_FORMAT))
		{
			error_invalid_exit("encoding");
		}
		nGlyphNames = read_int32();
		if (nGlyphNames != nMetrics)
		{
			error_exit("nGlyphNames != nMetrics");
		}
		for (i = 0; i < nGlyphNames; i++)
		{
			metrics[i].glyphName.v = read_int32();
		}
		glyphNamesSize = read_int32();
		glyphNames = (char *)malloc((glyphNamesSize + 1) * sizeof(*glyphNames));
		check_memory(glyphNames);
		read_byte8s((uint8_t *) glyphNames, glyphNamesSize);
		glyphNames[glyphNamesSize] = '\0';
		for (i = 0; i < nGlyphNames; i++)
		{
			if (glyphNamesSize <= metrics[i].glyphName.v)
			{
				error_invalid_exit("glyphNames");
			}
			metrics[i].glyphName.s = glyphNames + metrics[i].glyphName.v;
		}
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "no PCF_GLYPH_NAMES\n");
		}
	}

	/* read BDF style accelerators section */
	if (seek(PCF_BDF_ACCELERATORS))
	{
		if (verbose)
		{
			fprintf(stderr, "PCF_BDF_ACCELERATORS\n");
		}
		read_accelerators();
	} else
	{
		if (verbose)
		{
			fprintf(stderr, "no PCF_BDF_ACCELERATORS\n");
		}
	}

	/* write bdf file */

	fprintf(ofp, "STARTFONT 2.1\n");
	fprintf(ofp, "FONT %s\n", get_property_string("FONT"));
	int32_t rx = get_property_value("RESOLUTION_X");

	int32_t ry = get_property_value("RESOLUTION_Y");

	if (!is_exist_property_value("RESOLUTION_X") || !is_exist_property_value("RESOLUTION_Y"))
	{
		rx = ry = (int) (get_property_value("RESOLUTION") / 100.0 * 72.27);
	}
	fprintf(ofp, "SIZE %d %d %d\n", get_property_value("POINT_SIZE") / 10, rx, ry);
	fprintf(ofp, "FONTBOUNDINGBOX %d %d %d %d\n\n",
			widthBits(&fontbbx), height(&fontbbx), fontbbx.leftSideBearing, -fontbbx.descent);

	int nPropsd = -1;

	if (!is_exist_property_value("DEFAULT_CHAR") && defaultCh != NO_SUCH_CHAR)
	{
		nPropsd++;
	}
	if (!is_exist_property_value("FONT_DESCENT"))
	{
		nPropsd++;
	}
	if (!is_exist_property_value("FONT_ASCENT"))
	{
		nPropsd++;
	}
	if (is_exist_property_value("RESOLUTION_X") &&
		is_exist_property_value("RESOLUTION_Y") && is_exist_property_value("RESOLUTION"))
	{
		nPropsd--;
	}

	fprintf(ofp, "STARTPROPERTIES %d\n", nProps + nPropsd);
	for (i = 0; i < nProps; i++)
	{
		if (strcmp(props[i].name.s, "FONT") == 0)
		{
			continue;
		} else if (strcmp(props[i].name.s, "RESOLUTION") == 0 &&
				   is_exist_property_value("RESOLUTION_X") && is_exist_property_value("RESOLUTION_Y"))
		{
			continue;
		}
		fprintf(ofp, "%s ", props[i].name.s);
		if (props[i].isStringProp)
		{
			fprintf(ofp, "\"");
			for (const char *p = props[i].value.s; *p; ++p)
			{
				if (*p == '"')
				{
					fprintf(ofp, "\"");
				}
				fprintf(ofp, "%c", *p);
			}
			fprintf(ofp, "\"\n");
		} else
		{
			fprintf(ofp, "%d\n", props[i].value.v);
		}
	}

	if (!is_exist_property_value("DEFAULT_CHAR") && defaultCh != NO_SUCH_CHAR)
	{
		fprintf(ofp, "DEFAULT_CHAR %d\n", defaultCh);
	}
	if (!is_exist_property_value("FONT_DESCENT"))
	{
		fprintf(ofp, "FONT_DESCENT %d\n", accelerators.fontDescent);
	}
	if (!is_exist_property_value("FONT_ASCENT"))
	{
		fprintf(ofp, "FONT_ASCENT %d\n", accelerators.fontAscent);
	}
	fprintf(ofp, "ENDPROPERTIES\n\n");

	fprintf(ofp, "CHARS %d\n\n", nValidEncodings);

	for (i = 0; i < nEncodings; i++)
	{
		if (encodings[i] == NO_SUCH_CHAR)
		{
			continue;
		}

		int col = i % (lastCol - firstCol + 1) + firstCol;

		int row = i / (lastCol - firstCol + 1) + firstRow;

		uint16_t charcode = make_charcode(row, col);

		struct metric_t *m = &metrics[encodings[i]];
		if (m->glyphName.s)
		{
			fprintf(ofp, "STARTCHAR %s\n", m->glyphName.s);
		} else if (0x21 <= charcode && charcode <= 0x7e)
		{
			fprintf(ofp, "STARTCHAR %c\n", (char) charcode);
		} else
		{
			fprintf(ofp, "STARTCHAR %04X\n", charcode);
		}
		fprintf(ofp, "ENCODING %d\n", charcode);
		fprintf(ofp, "SWIDTH %d %d\n", m->swidth, 0);
		fprintf(ofp, "DWIDTH %d %d\n", m->characterWidth, 0);
		fprintf(ofp, "BBX %d %d %d %d\n", widthBits(m), height(m), m->leftSideBearing, -m->descent);
		if (0 < m->attributes)
		{
			fprintf(ofp, "ATTRIBUTES %4X\n", (uint16_t) m->attributes);
		}
		fprintf(ofp, "BITMAP\n");

		int Bytes = widthBytes(m, &format);

		int w = (widthBits(m) + 7) / 8;

		w = w < 1 ? 1 : w;
		uint8_t *b = m->bitmaps;

		for (int r = 0; r < height(m); r++)
		{
			for (int c = 0; c < Bytes; c++)
			{
				if (c < w)
				{
					fprintf(ofp, "%02X", *b);
				}
				b++;
			}
			fprintf(ofp, "\n");
		}
		fprintf(ofp, "ENDCHAR\n\n");
	}

	fprintf(ofp, "ENDFONT\n");
	return 0;
}
