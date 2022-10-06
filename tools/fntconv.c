/*
 * fntconv.c : convert Gemdos fonts between FNT, TXT and C formats
 *
 * Copyright (c) 2001 Laurent Vogel
 * Copyright (c) 2017 Thorsten Otto
 *
 * Authors:
 *  LVL     Laurent Vogel
 *  THO     Thorsten Otto
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

#include "fontdef.h"
#include "array.h"
#include "stcharmap.h"

#define FILE_C 1
#define FILE_TXT 2
#define FILE_FNT 3
#define FILE_BMP 4
#define FILE_C16 5
#define FILE_CRX 6
#define FILE_BGI 7
#define FILE_TTF 8
static int convert_from;
static int convert_to;
static int scale = 1;
static int grid = 0;

#define MAX_ADE 0x7fffl

static int all_chars = 0;
static int for_aranym = 0;
static int for_plain = 0;
static int do_off_table = 1;
static const char *varname = "THISFONT";


/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

/*
 * internal error routine
 */

static void fatal(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\naborted.\n");
	va_end(ap);
	exit(EXIT_FAILURE);
}

/* -------------------------------------------------------------------------- */

/*
 * memory
 */
void *xmalloc(size_t s)
{
	void *a = calloc(1, s);

	if (a == 0)
		fatal("%s", strerror(errno));
	return a;
}

/* -------------------------------------------------------------------------- */

void *xrealloc(void *p, size_t size)
{
	void *a = realloc(p, size);

	if (a == 0)
		fatal("%s", strerror(errno));
	return a;
}

/* -------------------------------------------------------------------------- */

static struct font *font_alloc(void)
{
	struct font *p;

	p = (struct font *)xmalloc(sizeof(*p));
	return p;
}

/* -------------------------------------------------------------------------- */

/*
 * xstrdup
 */

static char *xstrdup(const char *s)
{
	size_t len = strlen(s);
	char *a = (char *)xmalloc(len + 1);

	strcpy(a, s);
	return a;
}

/* -------------------------------------------------------------------------- */

/*
 * little/big endian conversion
 */

static int32_t get_b_long(void *addr)
{
	uint8_t *uaddr = (uint8_t *) addr;

	return (uaddr[0] << 24) + (uaddr[1] << 16) + (uaddr[2] << 8) + uaddr[3];
}

/* -------------------------------------------------------------------------- */

static void set_b_long(void *addr, int32_t value)
{
	uint8_t *uaddr = (uint8_t *) addr;

	uaddr[0] = value >> 24;
	uaddr[1] = value >> 16;
	uaddr[2] = value >> 8;
	uaddr[3] = value;
}

/* -------------------------------------------------------------------------- */

static int16_t get_b_word(void *addr)
{
	uint8_t *uaddr = (uint8_t *) addr;

	return (uaddr[0] << 8) + uaddr[1];
}

/* -------------------------------------------------------------------------- */

static void set_b_word(void *addr, int16_t value)
{
	uint8_t *uaddr = (uint8_t *) addr;

	uaddr[0] = value >> 8;
	uaddr[1] = value;
}

/* -------------------------------------------------------------------------- */

static int32_t get_l_long(void *addr)
{
	uint8_t *uaddr = (uint8_t *) addr;

	return (uaddr[3] << 24) + (uaddr[2] << 16) + (uaddr[1] << 8) + uaddr[0];
}

/* -------------------------------------------------------------------------- */

static void set_l_long(void *addr, int32_t value)
{
	uint8_t *uaddr = (uint8_t *) addr;

	uaddr[3] = value >> 24;
	uaddr[2] = value >> 16;
	uaddr[1] = value >> 8;
	uaddr[0] = value;
}

/* -------------------------------------------------------------------------- */

static void set_l_word(void *addr, int16_t value)
{
	uint8_t *uaddr = (uint8_t *) addr;

	uaddr[1] = value >> 8;
	uaddr[0] = value;
}

/* -------------------------------------------------------------------------- */

static int16_t get_l_word(void *addr)
{
	uint8_t *uaddr = (uint8_t *) addr;

	return (uaddr[1] << 8) + uaddr[0];
}

/* -------------------------------------------------------------------------- */

#if 0
static void set_l_word(void *addr, int16_t value)
{
	uint8_t *uaddr = (uint8_t *) addr;

	uaddr[1] = value >> 8;
	uaddr[0] = value;
}
#endif

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

static FILE *open_output(const char **filename, const char *mode)
{
	FILE *f;
	
	if (*filename == NULL || strcmp(*filename, "-") == 0)
	{
		fflush(stdout);
#ifdef __PUREC__
		f = stdout;
#else
		f = fdopen(fileno(stdout), mode);
#endif
		*filename = "<stdout>";
	} else
	{
		f = fopen(*filename, mode);
	}
	if (f == NULL)
		fatal("can't create %s: %s", *filename, strerror(errno));
	return f;
}

/* -------------------------------------------------------------------------- */

static void close_output(FILE *f)
{
	fflush(f);
	if (f != stdout && fclose(f) != 0)
		fatal("fclose");
}

/* -------------------------------------------------------------------------- */

static FILE *open_input(const char **filename, const char *mode)
{
	FILE *f;
	
	if (*filename == NULL || strcmp(*filename, "-") == 0)
	{
#ifdef __PUREC__
		f = stdin;
#else
		f = fdopen(fileno(stdin), mode);
#endif
		*filename = "<stdin>";
	} else
	{
		f = fopen(*filename, mode);
	}
	if (f == NULL)
		fatal("can't open %s: %s", *filename, strerror(errno));
	return f;
}

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

/*
 * text input files
 */

#define BACKSIZ 10
#define READSIZ 512

typedef struct ifile
{
	long lineno;
	char *fname;
	FILE *fh;
	uint8_t buf[BACKSIZ + READSIZ];
	long size;
	long index;
	int ateof;
} IFILE;

/* -------------------------------------------------------------------------- */

static void irefill(IFILE *f)
{
	if (f->size > BACKSIZ)
	{
		memmove(f->buf, f->buf + f->size - BACKSIZ, BACKSIZ);
		f->size = BACKSIZ;
		f->index = f->size;
	}
	f->size += fread(f->buf + f->size, 1, READSIZ, f->fh);
}

/* -------------------------------------------------------------------------- */

static IFILE *ifopen(const char *fname)
{
	IFILE *f = (IFILE *)xmalloc(sizeof(IFILE));

	f->fh = open_input(&fname, "rb");
	if (f->fh == NULL)
	{
		free(f);
		return NULL;
	}
	f->fname = xstrdup(fname);
	f->size = 0;
	f->index = 0;
	f->ateof = 0;
	f->lineno = 1;
	return f;
}

/* -------------------------------------------------------------------------- */

static void ifclose(IFILE *f)
{
	fclose(f->fh);
	free(f);
}

/* -------------------------------------------------------------------------- */

static void iback(IFILE *f)
{
	if (f->index == 0)
	{
		fatal("too far backward");
	} else
	{
		f->index--;
	}
}

/* -------------------------------------------------------------------------- */

static int igetc(IFILE *f)
{
	if (f->index >= f->size)
	{
		irefill(f);
		if (f->index >= f->size)
		{
			f->ateof = 1;
			return EOF;
		}
	}
	return f->buf[f->index++];
}

/* -------------------------------------------------------------------------- */

/* returns the next logical char, in sh syntax */
static int inextsh(IFILE *f)
{
	int ret;

	ret = igetc(f);
	if (ret == 015)
	{
		ret = igetc(f);
		if (ret == 012)
		{
			f->lineno++;
			return '\n';
		} else
		{
			iback(f);
			f->lineno++;
			return '\n';
		}
	} else if (ret == 012)
	{
		f->lineno++;
		return '\n';
	} else
	{
		return ret;
	}
}

/* -------------------------------------------------------------------------- */

/* read a line, ignoring comments, initial white, trailing white,
 * empty lines and long lines 
 */
static int igetline(IFILE *f, char *buf, int max)
{
	char *b;
	char *bmax = buf + max - 1;
	int c;

  again:
	c = inextsh(f);
	b = buf;
	if (c == '#')
	{
	  ignore:
		while (c != EOF && c != '\n')
		{
			c = inextsh(f);
		}
		goto again;
	}
	while (c == ' ' || c == '\t')
	{
		c = inextsh(f);
	}
	while (c != EOF && c != '\n')
	{
		if (b >= bmax)
		{
			fprintf(stderr, "file %s, line %ld too long\n", f->fname, f->lineno);
			goto ignore;
		}
		*b++ = c;
		c = inextsh(f);
	}
	/* remove trailing white */
	if (b == buf)
		return 0;						/* EOF */
	b--;
	while (b >= buf && (*b == ' ' || *b == '\t'))
	{
		b--;
	}
	b++;
	*b = 0;
	return 1;
}

/* -------------------------------------------------------------------------- */

/*
 * functions to try read some patterns.
 * they look in a string, return 1 if read, 0 if not read.
 * if the pattern was read, the string pointer is set to the
 * character immediately after the last character of the pattern.
 */

/* backslash sequences in C strings */
static int try_backslash(char **cc, long *val)
{
	long ret;
	char *c = *cc;

	if (*c++ != '\\')
		return 0;
	switch (*c)
	{
	case 0:
		return 0;
	case 'a':
		ret = '\a';
		c++;
		break;
	case 'b':
		ret = '\b';
		c++;
		break;
	case 'f':
		ret = '\f';
		c++;
		break;
	case 'n':
		ret = '\n';
		c++;
		break;
	case 'r':
		ret = '\r';
		c++;
		break;
	case 't':
		ret = '\t';
		c++;
		break;
	case 'v':
		ret = '\v';
		c++;
		break;
	case '\\':
	case '\'':
	case '\"':
		ret = *c++;
		break;
	default:
		if (*c >= '0' && *c <= '7')
		{
			ret = *c++ - '0';
			if (*c >= '0' && *c <= '7')
			{
				ret <<= 3;
				ret |= *c++ - '0';
				if (*c >= '0' && *c <= '7')
				{
					ret <<= 3;
					ret |= *c++ - '0';
				}
			}
		} else
		{
			ret = *c++;
		}
		break;
	}
	*cc = c;
	*val = ret;
	return 1;
}

/* -------------------------------------------------------------------------- */

static int try_unsigned(char **cc, long *val)
{
	long ret;
	char *c = *cc;

	if (*c == '0')
	{
		c++;
		if (*c == 'x')
		{
			c++;
			ret = 0;
			if (*c == 0)
				return 0;
			while (*c)
			{
				if (*c >= '0' && *c <= '9')
				{
					ret <<= 4;
					ret |= (*c - '0');
				} else if (*c >= 'a' && *c <= 'f')
				{
					ret <<= 4;
					ret |= (*c - 'a' + 10);
				} else if (*c >= 'A' && *c <= 'F')
				{
					ret <<= 4;
					ret |= (*c - 'A' + 10);
				} else
					break;
				c++;
			}
		} else
		{
			ret = 0;
			while (*c >= '0' && *c <= '7')
			{
				ret <<= 3;
				ret |= (*c++ - '0');
			}
		}
	} else if (*c >= '1' && *c <= '9')
	{

		ret = 0;
		while (*c >= '0' && *c <= '9')
		{
			ret *= 10;
			ret += (*c++ - '0');
		}
	} else if (*c == '\'')
	{
		c++;
		if (try_backslash(&c, &ret))
		{
			if (*c++ != '\'')
				return 0;
		} else if (*c == '\'' || *c < 32 || *c >= 127)
		{
			return 0;
		} else
		{
			ret = (*c++) & 0xFF;
			if (*c++ != '\'')
				return 0;
		}
	} else
		return 0;
	*cc = c;
	*val = ret;
	return 1;
}

/* -------------------------------------------------------------------------- */

#if 0
static int try_signed(char **cc, long *val)
{
	char *c = *cc;

	if (*c == '-')
	{
		c++;
		if (try_unsigned(&c, val))
		{
			*val = -*val;
			*cc = c;
			return 1;
		} else
		{
			return 0;
		}
	} else
	{
		return try_unsigned(cc, val);
	}
}
#endif

/* -------------------------------------------------------------------------- */

static int try_given_string(char **cc, char *s)
{
	size_t n = strlen(s);

	if (!strncmp(*cc, s, n))
	{
		*cc += n;
		return 1;
	} else
	{
		return 0;
	}
}

/* -------------------------------------------------------------------------- */

static int try_c_string(char **cc, char *s, int max)
{
	char *c = *cc;
	char *smax = s + max - 1;
	long u;

	if (*c != '"')
	{
		fprintf(stderr, "c='%c'\n", *c);
		return 0;
	}
	c++;
	while (*c != '"')
	{
		if (*c == 0)
		{
			fprintf(stderr, "c='%c'\n", *c);
			return 0;
		}
		if (s >= smax)
		{
			fprintf(stderr, "c='%c'\n", *c);
			return 0;
		}
		if (try_backslash(&c, &u))
		{
			*s++ = u;
		} else
		{
			*s++ = *c++;
		}
	}
	c++;
	*s++ = 0;
	*cc = c;
	return 1;
}

/* -------------------------------------------------------------------------- */

static int try_white(char **cc)
{
	char *c = *cc;

	if (*c == ' ' || *c == '\t')
	{
		c++;
		while (*c == ' ' || *c == '\t')
			c++;
		*cc = c;
		return 1;
	} else
		return 0;
}

/* -------------------------------------------------------------------------- */

static int try_eol(char **cc)
{
	return (**cc == 0) ? 1 : 0;
}

/* -------------------------------------------------------------------------- */

/*
 * simple bitmap read/write
 */

static int get_bit(uint8_t *addr, long i)
{
	return (addr[i >> 3] & (1 << (7 - (i & 7)))) ? 1 : 0;
}

/* -------------------------------------------------------------------------- */

static void set_bit(uint8_t *addr, long i)
{
	addr[i >> 3] |= (1 << (7 - (i & 7)));
}

/* -------------------------------------------------------------------------- */

static unsigned long get_width(struct font *p, unsigned short ch)
{
	unsigned short next;
	unsigned short bmnum = p->last_ade - p->first_ade + 1;
	unsigned long off;
	
	off = p->off_table[ch];
	if (off != F_NO_CHARL)
	{
		for (next = ch + 1; next <= bmnum; next++)
		{
			unsigned long nextoff = p->off_table[next];
			if (nextoff != F_NO_CHARL)
				return nextoff - off;
		}
		off = F_NO_CHARL;
	}
	return off;
}

/* -------------------------------------------------------------------------- */

static void check_monospaced(struct font *p, const char *filename)
{
	int i, bmnum;
	int mono = 1;
	unsigned long width, firstwidth;
	
	bmnum = p->last_ade - p->first_ade + 1;
	firstwidth = get_width(p, 0);
	for (i = 0; i < bmnum; i++)
	{
		width = get_width(p, i);
		if (width == F_NO_CHARL)
			continue;
		if (width != 0 && width != firstwidth)
		{
			mono = 0;
			if (p->flags & F_MONOSPACE)
				fprintf(stderr, "%s: warning: font says it is monospaced, but isn't\n", filename);
			p->flags &= ~F_MONOSPACE;
			return;
		}
	}
	if (mono && !(p->flags & F_MONOSPACE))
	{
		fprintf(stderr, "%s: warning: font does not say it is monospaced, but is\n", filename);
		p->flags |= F_MONOSPACE;
	}
}

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

/*
 * read functions
 */

static struct font *read_txt(const char *fname)
{
	IFILE *f;
	int ch, i, j, k, lastch;
	unsigned long off;
	int height;
	unsigned long width = 0;
	int w;
	uint8_t *bms;
	int bmsize, bmnum;
	int first, last;
	uint8_t *b;
	char *c;
	long u;
	char line[200];
	struct font *p;
	int max = 80;

	p = font_alloc();
	f = ifopen(fname);

#define EXPECT(a) \
  if(!igetline(f, line, max) || strcmp(line, a) != 0) \
  { \
    fprintf(stderr, "\"%s\" expected\n", a); \
  	goto fail; \
  }

	EXPECT("GDOSFONT");
	EXPECT("version 1.0");

#define EXPECTNUM(a) c=line; \
  if(!igetline(f, line, max) || !try_given_string(&c, #a) \
    || !try_white(&c) || !try_unsigned(&c, &u) || !try_eol(&c)) \
  { \
    fprintf(stderr, "\"%s\" with number expected\n", #a); \
    goto fail; \
  } \
  p->a = u;

	EXPECTNUM(font_id);
	EXPECTNUM(point);

	c = line;
	if (!igetline(f, line, max) || !try_given_string(&c, "name")
		|| !try_white(&c) || !try_c_string(&c, p->name, 32) || !try_eol(&c))
    {
    	fprintf(stderr, "\"%s\" expected\n", "name");
		goto fail;
	}

	EXPECTNUM(first_ade);
	EXPECTNUM(last_ade);
	EXPECTNUM(top);
	EXPECTNUM(ascent);
	EXPECTNUM(half);
	EXPECTNUM(descent);
	EXPECTNUM(bottom);
	EXPECTNUM(max_char_width);
	EXPECTNUM(max_cell_width);
	EXPECTNUM(left_offset);
	EXPECTNUM(right_offset);
	EXPECTNUM(thicken);
	EXPECTNUM(ul_size);
	EXPECTNUM(lighten);
	EXPECTNUM(skew);
	EXPECTNUM(flags);
	EXPECTNUM(form_height);

#undef EXPECTNUM

	if (p->flags & F_HORZ_OFF)
	{
		fatal("horizontal offsets not handled");
	}

	first = p->first_ade;
	last = p->last_ade;
	height = p->form_height;
	if (first < 0 || last < 0 || (long)first > MAX_ADE || first > last)
	{
		fatal("wrong char range : first = %d, last = %d", first, last);
	}
	if (p->max_cell_width >= 100 || height >= 100)
	{
		fatal("unreasonable font size %dx%d", p->max_cell_width, height);
	}

	/* allocate a big buffer to hold all bitmaps */
	bmnum = last - first + 1;
	bmsize = (p->max_cell_width * height + 7) >> 3;
	bms = (uint8_t *)xmalloc((size_t)bmsize * bmnum);
	p->off_table = (unsigned long *)xmalloc(sizeof(*p->off_table) * (bmnum + 1));
	for (i = 0; i < bmnum; i++)
	{
		p->off_table[i] = F_NO_CHARL;
	}

	lastch = 0;
	for (;;)
	{									/* for each char */
		c = line;
		if (!igetline(f, line, max))
		{
	    	fprintf(stderr, "unexpected EOF\n");
			goto fail;
		}
		if (strcmp(line, "endfont") == 0)
			break;
		if (!try_given_string(&c, "char") || !try_white(&c) || !try_unsigned(&c, &u) || !try_eol(&c))
		{
	    	fprintf(stderr, "\"%s\" with number expected\n", "char");
			goto fail;
		}
		ch = (int)u;
		if (ch < first || ch > last)
		{
			fprintf(stderr, "wrong character number 0x%x\n", ch);
			goto fail;
		}
		if (ch < lastch)
			fprintf(stderr, "%s:%ld: warning: char 0x%x less previous char 0x%x\n", fname, f->lineno, ch, lastch);
		lastch = ch;

		ch -= first;
		if (p->off_table[ch] != F_NO_CHARL)
		{
			fprintf(stderr, "%s:%ld: character number 0x%x was already defined\n", fname, f->lineno, lastch);
			goto fail;
		}
		b = bms + ch * bmsize;

		k = 0;
		for (i = 0; i < height; i++)
		{
			if (!igetline(f, line, max))
			{
		    	fprintf(stderr, "unexpected EOF\n");
				goto fail;
			}
			for (c = line, w = 0; *c; c++, w++)
			{
				if (w >= p->max_cell_width)
				{
					fprintf(stderr, "bitmap line to long at line %ld.", f->lineno);
					goto fail;
				} else if (*c == 'X')
				{
					set_bit(b, k);
					k++;
				} else if (*c == '.')
				{
					k++;
				} else
				{
					fprintf(stderr, "illegal character '%c' in bitmap definition\n", *c);
					goto fail;
				}
			}
			if (i == 0)
			{
				width = w;
			} else if ((unsigned long)w != width)
			{
				fprintf(stderr, "bitmap lines of different lengths\n");
				goto fail;
			}
		}
		EXPECT("endchar");
		p->off_table[ch] = width;			/* != F_NO_CHAR, real value filled later */
		if ((all_chars || (p->flags & F_MONOSPACE)) && width != p->max_cell_width)
		{
			fprintf(stderr, "%s: 0x%x: width %ld != cell width %d\n", fname, lastch, width, p->max_cell_width);
			goto fail;
		}
	}
	ifclose(f);
#undef EXPECT

	/* compute size of final form, and compute offs from widths */
	off = 0;
	for (i = 0; i < bmnum; i++)
	{
		width = p->off_table[i];
		p->off_table[i] = off;
		if (width != F_NO_CHARL)
		{
			off += width;
		} else
		{
			if (i < 256 && !(i >= 0x80 && i <= 0x9f))
			{
				fprintf(stderr, "%s: warning: %x undefined\n", fname, i);
			}
			if (all_chars)
			{
				off += p->max_cell_width;
			}
		}
	}
	p->off_table[bmnum] = off;
	if (convert_to == FILE_FNT && off >= 0x10000L)
	{
		fprintf(stderr, "%s: last offset %ld too large for generating GEM font\n", fname, off);
		exit(EXIT_FAILURE);
	}
	p->form_width = ((off + 15) >> 4) << 1;
	p->dat_table = (uint8_t *)xmalloc((size_t)height * p->form_width);

	check_monospaced(p, fname);
	
	/* now, pack the bitmaps in the destination form */
	for (i = 0; i < bmnum; i++)
	{
		off = p->off_table[i];
		width = get_width(p, i);
		if (width == F_NO_CHARL)
			continue;
		b = bms + bmsize * i;
		k = 0;
		for (j = 0; j < height; j++)
		{
			for (w = 0; w < (int)width; w++)
			{
				if (get_bit(b, k))
				{
					set_bit(p->dat_table + j * p->form_width, off + w);
				}
				k++;
			}
		}
	}

	/* free temporary form */
	free(bms);

	return p;
  fail:
	fprintf(stderr, "fatal error file %s line %ld\n", f->fname, f->lineno - 1);
	ifclose(f);
	exit(EXIT_FAILURE);
	return NULL;
}

/* -------------------------------------------------------------------------- */

static struct font *read_fnt(const char *fname)
{
	struct font *p;
	FILE *f;
	struct font_file_hdr h;
	long count;
	long off_hor_table;
	long off_off_table;
	long off_dat_table;
	int bigendian = 0;
	int bmnum;

	p = font_alloc();
	f = open_input(&fname, "rb");

	count = fread(&h, 1, sizeof(h), f);
	if (count != sizeof(h))
		fatal("short fread");

	/* determine byte order */
	if (get_l_word(h.point) >= 256)
	{
		bigendian = 1;
	} else
	{
		bigendian = 0;
	}

	/* convert the header */
#define GET_WORD(a) \
  if(bigendian) p->a = get_b_word(h.a); else p->a = get_l_word(h.a);

	GET_WORD(font_id);
	GET_WORD(point);

	memcpy(p->name, h.name, sizeof(h.name));

	GET_WORD(first_ade);
	GET_WORD(last_ade);
	GET_WORD(top);
	GET_WORD(ascent);
	GET_WORD(half);
	GET_WORD(descent);
	GET_WORD(bottom);
	GET_WORD(max_char_width);
	GET_WORD(max_cell_width);
	GET_WORD(left_offset);
	GET_WORD(right_offset);
	GET_WORD(thicken);
	GET_WORD(ul_size);
	GET_WORD(lighten);
	GET_WORD(skew);
	GET_WORD(flags);

	if (bigendian)
	{
		off_hor_table = get_b_long(h.hor_table);
		off_off_table = get_b_long(h.off_table);
		off_dat_table = get_b_long(h.dat_table);
	} else
	{
		off_hor_table = get_l_long(h.hor_table);
		off_off_table = get_l_long(h.off_table);
		off_dat_table = get_l_long(h.dat_table);
	}

	GET_WORD(form_width);
	GET_WORD(form_height);

#undef GET_WORD

	/* make some checks */
	if (p->last_ade > MAX_ADE || p->first_ade > p->last_ade)
	{
		fatal("wrong char range : first = %d, last = %d", p->first_ade, p->last_ade);
	}
	if (p->max_cell_width >= 40 || p->form_height >= 40)
	{
		fatal("unreasonable font size %dx%d", p->max_cell_width, p->form_height);
	}

	if (fseek(f, off_off_table, SEEK_SET))
		fatal("seek");
	bmnum = p->last_ade - p->first_ade + 1;
	p->off_table = (unsigned long *)xmalloc(sizeof(*p->off_table) * (bmnum + 1));
	
	{
		int i;
		char buf[2];

		for (i = 0; i <= bmnum; i++)
		{
			count = 2;
			if ((long)fread(buf, 1, count, f) != count)
				fatal("short read");
			if (bigendian)
			{
				p->off_table[i] = get_b_word(buf);
			} else
			{
				p->off_table[i] = get_l_word(buf);
			}
			if (p->off_table[i] == F_NO_CHAR)
			{
				p->off_table[i] = F_NO_CHARL;
			} else
			{
				if (i == bmnum)
				{
					if ((((p->off_table[i] + 15) >> 4) << 1) != p->form_width)
					{
						fprintf(stderr, "warning: %s: offset of last character %lu does not match form_width %lu\n", fname, p->off_table[i], p->form_width);
	 					if (p->off_table[i] < p->off_table[i - 1])
							p->off_table[i] = p->form_width << 3;
					}
				}
				if (i != 0)
				{
					if (p->off_table[i] < p->off_table[i - 1])
						fprintf(stderr, "warning: %s: corrupted offset table at %u: %lu < %lu\n", fname, i, p->off_table[i], p->off_table[i - 1]);
				}
			}
		}
	}
	if (fseek(f, off_dat_table, SEEK_SET))
		fatal("seek");
	count = p->form_height * p->form_width;
	p->dat_table = (uint8_t *)xmalloc(count);
	if ((long)fread(p->dat_table, 1, count, f) != count)
		fatal("short read");

	if (p->flags & F_HORZ_OFF)
	{
		long i;
		int empty;
		
		/*
		 * check wether offset table is really present;
		 * some fonts have this incorrectly set
		 */
		count = 2 * bmnum;
		if (off_hor_table == 0 || off_hor_table == off_off_table)
		{
			fprintf(stderr, "%s: ignoring non-existing offset table\n", fname);
			p->flags &= ~F_HORZ_OFF;
		} else if ((off_off_table - off_hor_table) < count)
		{
			fprintf(stderr, "%s: ignoring offset table of size %ld, should be %ld\n", fname, off_off_table - off_hor_table, count);
			p->flags &= ~F_HORZ_OFF;
		} else
		{
			p->hor_table = (uint8_t *)xmalloc(count);
			if (fseek(f, off_hor_table, SEEK_SET))
				fatal("seek");
			if ((long)fread(p->hor_table, 1, count, f) != count)
				fatal("short read");
			empty = 1;
			for (i = 0; i < count; i++)
			{
				if (p->hor_table[i] != 0)
					empty = 0;
			}
			if (empty)
			{
				fprintf(stderr, "%s: warning: empty horizontal offset table\n", fname);
				p->flags &= ~F_HORZ_OFF;
			}
		}
	}
	fclose(f);

	check_monospaced(p, fname);
	
	return p;
}

/* -------------------------------------------------------------------------- */

static void write_fnt(struct font *p, const char *fname)
{
	FILE *f;
	char buf[2];
	int i;
	struct font_file_hdr h;
	long count;
	long off_hor_table;
	long off_off_table;
	long off_dat_table;
	long off;
	int bmnum;
	
	bmnum = p->last_ade - p->first_ade + 1;

	if (p->off_table[bmnum] >= 0x10000L || (unsigned long)p->form_height >= 0xFFFFUL)
	{
		fprintf(stderr, "%s: form width %ld too large for GEM font\n", fname, p->off_table[bmnum]);
		exit(EXIT_FAILURE);
	}

	f = open_output(&fname, "wb");

	p->flags |= F_STDFORM;
	
#define SET_WORD(a) set_b_word(h.a, p->a)
	SET_WORD(font_id);
	SET_WORD(point);
	memcpy(h.name, p->name, sizeof(h.name));

	SET_WORD(first_ade);
	SET_WORD(last_ade);
	SET_WORD(top);
	SET_WORD(ascent);
	SET_WORD(half);
	SET_WORD(descent);
	SET_WORD(bottom);
	SET_WORD(max_char_width);
	SET_WORD(max_cell_width);
	SET_WORD(left_offset);
	SET_WORD(right_offset);
	SET_WORD(thicken);
	SET_WORD(ul_size);
	SET_WORD(lighten);
	SET_WORD(skew);
	SET_WORD(flags);
	SET_WORD(form_width);
	SET_WORD(form_height);
#undef SET_WORD

	off = sizeof(struct font_file_hdr);
	off_hor_table = off;
	if (p->flags & F_HORZ_OFF)
	{
		off += bmnum * 2;
	}
	off_off_table = off;
	off += (bmnum + 1) * 2;
	off_dat_table = off;

	set_b_long(h.hor_table, off_hor_table);
	set_b_long(h.off_table, off_off_table);
	set_b_long(h.dat_table, off_dat_table);
	set_b_long(h.next_font, 0);

	count = sizeof(h);
	if (count != (long)fwrite(&h, 1, count, f))
		fatal("write");
	if (p->flags & F_HORZ_OFF)
	{
		i = bmnum * 2;
		if (i != (int)fwrite(p->hor_table, 1, i, f))
			fatal("write");
	}
	for (i = 0; i <= bmnum; i++)
	{
		set_b_word(buf, p->off_table[i]);
		if (2 != fwrite(buf, 1, 2, f))
			fatal("write");
	}
	count = p->form_width * p->form_height;
	if (count != (long)fwrite(p->dat_table, 1, count, f))
		fatal("write");
	close_output(f);
}

/* -------------------------------------------------------------------------- */

static void write_bmp(struct font *p, const char *fname)
{
	FILE *f;
	long bmpwidth;
	long bmpstride;
	long bmpheight;
	int bmnum, i;
	int charrows;
	long datasize;
	int cmapsize = 256 * 4;
	unsigned char *bitmap;
	
#define CHAR_COLUMNS 16
	
	struct {
		unsigned char magic[2];			/* BM */
		unsigned char filesize[4];
		unsigned char xHotSpot[2];     
		unsigned char yHotSpot[2];
		unsigned char offbits[4];		/* offset to data */
	} fileheader;
	struct {
		unsigned char bisize[4];
		unsigned char width[4];
		unsigned char height[4];
		unsigned char planes[2];
		unsigned char bitcount[2];
		unsigned char compressed[4];
		unsigned char datasize[4];
		unsigned char pix_width[4];
		unsigned char pix_height[4];
		unsigned char clr_used[4];
		unsigned char clr_important[4];
	} bmpheader;
	unsigned char palette[256][4];
	
	if (p->flags & F_HORZ_OFF)
	{
		fatal("horizontal offsets not handled");
	}

	bmnum = p->last_ade + 1;
	charrows = (bmnum + CHAR_COLUMNS - 1) / CHAR_COLUMNS;
	bmpwidth = CHAR_COLUMNS * p->max_cell_width * scale + (CHAR_COLUMNS + 1) * grid;
	bmpstride = (bmpwidth + 3) & ~3;
	bmpheight = charrows * p->form_height * scale + (charrows + 1) * grid;
	datasize = bmpstride * bmpheight;
	
	set_b_word(fileheader.magic, 0x424d);
	set_l_long(fileheader.filesize, sizeof(fileheader) + sizeof(bmpheader) + cmapsize + datasize);
	set_l_word(fileheader.xHotSpot, 0);
	set_l_word(fileheader.yHotSpot, 0);
	set_l_long(fileheader.offbits, sizeof(fileheader) + sizeof(bmpheader) + cmapsize);
	
	set_l_long(bmpheader.bisize, sizeof(bmpheader));
	set_l_long(bmpheader.width, bmpwidth);
	set_l_long(bmpheader.height, -bmpheight); /* we write it top-down */
	set_l_word(bmpheader.planes, 1);
	set_l_word(bmpheader.bitcount, 8);
	set_l_long(bmpheader.compressed, 0);
	set_l_long(bmpheader.datasize, datasize);
	set_l_long(bmpheader.pix_width, 95);
	set_l_long(bmpheader.pix_height, 95);
	set_l_long(bmpheader.clr_used, cmapsize / 4);
	set_l_long(bmpheader.clr_important, grid > 0 ? 3 : 2);
	memset(palette, 0, sizeof(palette));
	palette[0][0] = 255;
	palette[0][1] = 255;
	palette[0][2] = 255;
	palette[1][0] = 0;
	palette[1][1] = 0;
	palette[1][2] = 0;
	palette[2][0] = 0;
	palette[2][1] = 0;
	palette[2][2] = 255;
	
	f = open_output(&fname, "wb");

	bitmap = (unsigned char *)xmalloc(datasize);
	
	if (grid > 0)
	{
		int x, y, sy, sx;
		
		for (y = 0; y < (charrows + 1); y++)
		{
			for (sy = 0; sy < grid; sy++)
				for (x = 0; x < bmpwidth; x++)
					bitmap[((y * (p->form_height * scale + grid)) + sy) * bmpstride + x] = 2;
		}
		for (x = 0; x < (CHAR_COLUMNS + 1); x++)
		{
			for (sx = 0; sx < grid; sx++)
				for (y = 0; y < bmpheight; y++)
					bitmap[y * bmpstride + x * (p->max_cell_width * scale + grid) + sx] = 2;
		}
	}
	
	for (i = 0; i < bmnum; i++)
	{
		unsigned long w;
		unsigned long off;
		int y0;
		int x0;
		unsigned long x;
		uint16_t y;
		int sx, sy;
		
		if (i < p->first_ade)
			continue;
		off = p->off_table[i - p->first_ade];
		w = get_width(p, i - p->first_ade);
		if (w == F_NO_CHARL)
			continue;
		y0 = i / CHAR_COLUMNS;
		x0 = i % CHAR_COLUMNS;
		for (y = 0; y < p->form_height; y++)
		{
			for (x = 0; x < w; x++)
			{
				if (get_bit(p->dat_table + p->form_width * y, off + x))
				{
					for (sx = 0; sx < scale; sx++)
						for (sy = 0; sy < scale; sy++)
							bitmap[((y0 * p->form_height + y) * scale + y0 * grid + grid + sy) * bmpstride + ((x0 * p->max_cell_width + x) * scale) + x0 * grid + grid + sx] = 1;
				}
			}
		}
	}
	
	if (fwrite(&fileheader, 1, sizeof(fileheader), f) != sizeof(fileheader) ||
		fwrite(&bmpheader, 1, sizeof(bmpheader), f) != sizeof(bmpheader) ||
		(int)fwrite(palette, 1, cmapsize, f) != cmapsize ||
		(long)fwrite(bitmap, 1, datasize, f) != datasize)
		fatal("write");
		
	close_output(f);

#undef CHAR_COLUMNS
}

/* -------------------------------------------------------------------------- */

#include "truetype.h"

static void write_ttf(struct font *font, const char *fname)
{
	FILE *fp;
	uint32_t i;
	int id;
	unsigned long w;
	
	fp = open_output(&fname, "wb");
	font->glyphs = (struct glyph *)xmalloc(MAX_GLYPH * sizeof(*font->glyphs));
	for (i = 0; i < MAX_GLYPH; i++)
	{
		font->glyphs[i].id = -1;
		font->glyphs[i].idx = 0;
		font->glyphs[i].width = 0;
	}
	for (i = font->first_ade; i <= font->last_ade; i++)
	{
		if ((w = get_width(font, i - font->first_ade)) != F_NO_CHARL)
		{
			unsigned short uni;
			
			uni = i < 0x80 || i >= 0x100 ? i : known_pairs[i - 0x80].uni;
			font->glyphs[uni].idx = i - font->first_ade;
			font->glyphs[uni].width = (int)w;
			font->glyphs[uni].bbx.width = (int)w;
			font->glyphs[uni].bbx.height = font->form_height;
			font->glyphs[uni].bbx.x = 0;
			font->glyphs[uni].bbx.y = font->top - font->form_height;
		}
	}
	id = 0;
	for (i = 0; i < MAX_GLYPH; i++)
	{
		if (font->glyphs[i].width != 0)
		{
			font->glyphs[i].id = id;
			id++;
		}
	}
	font->num_glyphs = id;
	ttf_output(&font, 1, fp);

	close_output(fp);
	free(font->glyphs);
	font->glyphs = NULL;
}

/* -------------------------------------------------------------------------- */

static void write_txt(struct font *p, const char *filename)
{
	FILE *f;
	int i;

	/* first, write header */

	if (p->flags & F_HORZ_OFF)
	{
		fatal("horizontal offsets not handled");
	}

	f = open_output(&filename, "w");
	fprintf(f, "GDOSFONT\n");
	fprintf(f, "version 1.0\n");

#define SET_WORD(a) fprintf(f, #a " %u\n", p->a)
	SET_WORD(font_id);
	SET_WORD(point);

	fprintf(f, "name \"");
	for (i = 0; i < 32; i++)
	{
		char c = p->name[i];

		if (c == 0)
			break;
		if (c < 32 || c > 126 || c == '\\' || c == '"')
		{
			fprintf(f, "\\%03o", c);
		} else
		{
			fprintf(f, "%c", c);
		}
	}
	fprintf(f, "\"\n");

	SET_WORD(first_ade);
	SET_WORD(last_ade);
	SET_WORD(top);
	SET_WORD(ascent);
	SET_WORD(half);
	SET_WORD(descent);
	SET_WORD(bottom);
	SET_WORD(max_char_width);
	SET_WORD(max_cell_width);
	SET_WORD(left_offset);
	SET_WORD(right_offset);
	SET_WORD(thicken);
	SET_WORD(ul_size);

	fprintf(f, "lighten 0x%04x\n", p->lighten);
	fprintf(f, "skew 0x%04x\n", p->skew);
	fprintf(f, "flags 0x%02x\n", p->flags);

	SET_WORD(form_height);
#undef SET_WORD

	/* then, output char bitmaps */
	for (i = p->first_ade; i <= p->last_ade; i++)
	{
		uint16_t r;
		unsigned long c;
		unsigned long w;
		unsigned long off;

		off = p->off_table[i - p->first_ade];
		w = get_width(p, i - p->first_ade);
		if (w == F_NO_CHARL)
			continue;
		if ((off + w) > (8 * p->form_width))
		{
			fprintf(stderr, "char %d: offset %ld + width %ld out of range (%ld)\n", i, off, w, 8 * p->form_width);
			continue;
		}
		if (i < 32 || i > 126)
		{
			fprintf(f, "char 0x%02x\n", i);
		} else if (i == '\\' || i == '\'')
		{
			fprintf(f, "char '\\%c'\n", i);
		} else
		{
			fprintf(f, "char '%c'\n", i);
		}

		for (r = 0; r < p->form_height; r++)
		{
			for (c = 0; c < w; c++)
			{
				if (get_bit(p->dat_table + p->form_width * r, off + c))
				{
					fprintf(f, "X");
				} else
				{
					fprintf(f, ".");
				}
			}
			fprintf(f, "\n");
		}
		fprintf(f, "endchar\n");
	}
	fprintf(f, "endfont\n");
	close_output(f);
}

/* -------------------------------------------------------------------------- */

static void write_c_emutos(struct font *p, const char *filename, int plain)
{
	FILE *f;
	unsigned long i;
	int bmnum;
	const char *off_table_name = "off_table";
	const char *hor_table_name = "0";
	
	bmnum = p->last_ade - p->first_ade + 1;
	if (p->off_table[bmnum] >= 0x10000L)
	{
		fprintf(stderr, "%s: form width %ld too large for GEM font\n", filename, p->off_table[bmnum]);
		exit(EXIT_FAILURE);
	}

	/* output is always done bigendian */
	
	p->flags |= F_STDFORM;
	
	/* first, write header */

	f = open_output(&filename, "w");
	fprintf(f, "\
/*\n\
 * %s - a font in standard format\n\
 *\n\
 * Copyright (C) 2001-2019 The EmuTOS development team\n\
 *\n\
 * This file is distributed under the GPL, version 2 or at your\n\
 * option any later version.  See doc/license.txt for details.\n\
 *\n\
 * Automatically generated by fntconv.c\n\
 */\n", filename);
	fprintf(f, "\
\n");

	if (plain)
		fprintf(f, "\
#include <stdint.h>\n\
#include \"fonthdr.h\"\n\
\n");
	else
		fprintf(f, "\
#include \"config.h\"\n\
#include \"portab.h\"\n\
#include \"fonthdr.h\"\n\
\n");
	
	if (p->first_ade == 0 && p->last_ade == 255 && (p->flags & F_MONOSPACE))
	{
		if (p->max_cell_width == 6 && p->form_height == 6)
			off_table_name = "off_6x6_table";
		else if (p->max_cell_width == 8 && p->form_height == 8)
			off_table_name = "off_8x8_table";
		else if (p->max_cell_width == 8 && p->form_height == 16)
			off_table_name = "off_8x16_table";
		else if (!do_off_table)
			fatal("unsupported font size for omitting offset table");
	}
			
	if (do_off_table)
	{
		fprintf(f, "static %s %s[] =\n{\n", plain ? "UWORD const" : "const UWORD", off_table_name);
		{
			for (i = 0; i <= (unsigned long)bmnum; i++)
			{
				if ((i & 7) == 0)
					fprintf(f, "    ");
				else
					fprintf(f, " ");
				fprintf(f, "0x%04lx", p->off_table[i]);
				if (i != (unsigned long)bmnum)
					fprintf(f, ",");
				if ((i & 7) == 7)
					fprintf(f, "\n");
			}
		}
		if ((i & 7) != 7)
			fprintf(f, "\n");
		fprintf(f, "};\n\n");
	} else
	{
		if (p->first_ade == 0 && p->last_ade == 255 && (p->flags & F_MONOSPACE) &&
			p->max_cell_width == 8 && p->form_height == 16)
		{
			fprintf(f, "extern %s %s[];\n", plain ? "UWORD const" : "const UWORD", "off_8x8_table");
			fprintf(f, "#define %s %s\n", off_table_name, "off_8x8_table");
		} else
		{
			fprintf(f, "extern %s %s[];\n", plain ? "UWORD const" : "const UWORD", off_table_name);
		}
		fprintf(f, "\n");
	}
	
	if (p->flags & F_HORZ_OFF)
	{
		hor_table_name = "hor_table";
		
		fprintf(f, "static %s %s[] =\n{\n", plain ? "int8_t const" : "const UBYTE", hor_table_name);
		{
			unsigned long count = bmnum * 2;
			
			for (i = 0; i < count; i++)
			{
				if ((i & 7) == 0)
					fprintf(f, "    ");
				else
					fprintf(f, " ");
				fprintf(f, "0x%02x", p->hor_table[i]);
				if (i != (count - 1))
					fprintf(f, ",");
				if ((i & 7) == 7)
					fprintf(f, "\n");
			}
		}
		if ((i & 7) != 7)
			fprintf(f, "\n");
		fprintf(f, "};\n\n");
	}
	
	fprintf(f, "static %s dat_table[] =\n{\n", plain ? "UWORD const" : "const UWORD");
	{
		unsigned long h;
		unsigned int a;

		h = (p->form_height * p->form_width) / 2;
		for (i = 0; i < h; i++)
		{
			if ((i & 7) == 0)
				fprintf(f, "    ");
			else
				fprintf(f, " ");
			a = (p->dat_table[2 * i] << 8) | (p->dat_table[2 * i + 1] & 0xFF);
			fprintf(f, "0x%04x", a);
			if (i != (h - 1))
				fprintf(f, ",");
			if ((i & 7) == 7)
				fprintf(f, "\n");
		}
		if ((i & 7) != 0)
			fprintf(f, "\n");
	}
	fprintf(f, "};\n");
	fprintf(f, "\n");

	fprintf(f, "const Fonthead %s = {\n", varname);

#define SET_WORD(a) fprintf(f, "    %u,  /* " #a " */\n", (unsigned int)p->a)
    fprintf(f, "    %u,                  /* WORD font_id */\n", p->font_id);
    fprintf(f, "    %u,%s                 /* WORD point */\n", p->point, p->point < 10 ? " " : "");

	fprintf(f, "    \"");
	for (i = 0; i < 32; i++)
	{
		char c = p->name[i];

		if (c == 0)
			break;
		if (c < 32 || c > 126 || c == '\\' || c == '"')
		{
			fprintf(f, "\\%03o", c);
		} else
		{
			fprintf(f, "%c", c);
		}
	}
	fprintf(f, "\", /* char name[32] */\n");

    fprintf(f, "    %u,                  /* WORD first_ade */\n", p->first_ade);
    fprintf(f, "    %u,                /* WORD last_ade */\n", p->last_ade);
    fprintf(f, "    %u,%s                 /* UWORD top */\n", p->top, p->top < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD ascent */\n", p->ascent, p->ascent < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD half */\n", p->half, p->half < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD descent */\n", p->descent, p->descent < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD bottom */\n", p->bottom, p->bottom < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD max_char_width */\n", p->max_char_width, p->max_char_width < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD max_cell_width */\n", p->max_cell_width, p->max_cell_width < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD left_offset */\n", p->left_offset, p->left_offset < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD right_offset */\n", p->right_offset, p->right_offset < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD thicken */\n", p->thicken, p->thicken < 10 ? " " : "");
    fprintf(f, "    %u,%s                 /* UWORD ul_size */\n", p->ul_size, p->ul_size < 10 ? " " : "");

	fprintf(f, "    0x%04x,             /* UWORD lighten */\n", p->lighten);
	fprintf(f, "    0x%04x,             /* UWORD skew */\n", p->skew);

	fprintf(f, "    F_STDFORM");
	if (p->flags & F_MONOSPACE)
		fprintf(f, " | F_MONOSPACE");
	if (p->flags & F_DEFAULT)
		fprintf(f, " | F_DEFAULT");
	if (p->flags & F_HORZ_OFF)
		fprintf(f, " | F_HORZ_OFF");
	if (p->flags & F_EXT_HDR)
		fprintf(f, " | F_EXT_HDR");
	if ((p->flags & ~F_SUPPORTED) != 0)
		fprintf(f, " | 0x%x", p->flags & ~F_SUPPORTED);
	fprintf(f, ",  /* UWORD flags */\n");
	fprintf(f, "    %s,                  /* UBYTE *hor_table */\n", hor_table_name);
	fprintf(f, "    %s,     /* UWORD *off_table */\n", off_table_name);
	fprintf(f, "    dat_table,          /* UWORD *dat_table */\n");

	fprintf(f, "    %u,%s                /* UWORD form_width */\n", (unsigned int)p->form_width, p->form_width < 10 ? " " : "");
	fprintf(f, "    %u,%s                 /* UWORD form_height */\n", (unsigned int)p->form_height, p->form_height < 10 ? " " : "");
#undef SET_WORD
	if (plain)
	{
		fprintf(f, "    0                   /* struct font * next_font */\n");
	} else
	{
		fprintf(f, "    0                   /* Fonthead * next_font */\n");
	}
	fprintf(f, "};\n");

	close_output(f);
}

/* -------------------------------------------------------------------------- */

static void write_c_aranym(struct font *p, const char *filename)
{
	FILE *f;
	
	if (p->flags & F_HORZ_OFF)
	{
		fatal("horizontal offsets not handled");
	}

	/* first, write header */

	f = open_output(&filename, "w");
	fprintf(f, "\
/*\n\
 * font.h - 8x16 font for Atari ST encoding\n\
 * modified for the SDL-GUI (added a radio button and a checkbox)\n\
 * converted using ./fntconv -a -o font.h aranym10.txt\n\
 *\n\
 * The fntconv utility can be used to convert the font to text, edit it, and convert it back.\n\
 *\n\
 * Copyright (C) 2007  ARAnyM development team\n\
 * Copyright (C) 2001, 02 The EmuTOS development team\n\
 *\n\
 * This program is free software; you can redistribute it and/or modify\n\
 * it under the terms of the GNU General Public License as published by\n\
 * the Free Software Foundation; either version 2 of the License, or\n\
 * (at your option) any later version.\n\
\n\
 * This program is distributed in the hope that it will be useful,\n\
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
 * GNU General Public License for more details.\n\
\n\
 * You should have received a copy of the GNU General Public License\n\
 * along with this program; if not, write to the Free Software\n\
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\
 */\n\
\n\
\n");
	fprintf(f, "#define FONTWIDTH %d\n", p->max_cell_width);
	fprintf(f, "#define FONTHEIGHT %d\n", p->form_height);
	fprintf(f, "#define FONTCHARS %u\n", p->last_ade + 1);
	fprintf(f, "#define FORM_WIDTH %lu\n", p->form_width);
	fprintf(f, "\n");
	fprintf(f, "static unsigned char const font_bits[] = {\n");

	{
		unsigned long i, h;
		unsigned char a;

		h = p->form_height * p->form_width;
		for (i = 0; i < h; i++)
		{
			if ((i & 15) == 0)
				fprintf(f, "    ");
			else
				fprintf(f, " ");
			a = p->dat_table[i];
			fprintf(f, "0x%02x", a);
			if (i != (h - 1))
				fprintf(f, ",");
			if ((i & 15) == 15)
				fprintf(f, "\n");
		}
		if ((i & 15) != 0)
			fprintf(f, "\n");
	}
	fprintf(f, "};\n");

	close_output(f);
}

/* -------------------------------------------------------------------------- */

static void write_eps_c16(struct font *p, const char *filename)
{
	FILE *f;
	char outbuf[256 * 16];
	int c, l;
	long count;
	int bmnum;
	
	if (p->first_ade > 255 || p->last_ade > 255)
	{
		fprintf(stderr, "%s: too many characters for C16\n", filename);
		exit(EXIT_FAILURE);
	}
	bmnum = p->last_ade - p->first_ade + 1;
	if (!(p->flags & F_MONOSPACE) || p->form_width != (unsigned long)((bmnum + 1) & ~1) || p->form_height != 16)
	{
		fprintf(stderr, "%s: need monospaced 8x16 font for C16\n", filename);
		exit(EXIT_FAILURE);
	}
	f = open_output(&filename, "wb");
	memset(outbuf, 0, sizeof(outbuf));
    for (c = p->first_ade; c <= p->last_ade; c++)
    {
        for (l = 0; l < 16; l++)
        {
            outbuf[c * 16 + l] = p->dat_table[l * p->form_width + c - p->first_ade];
        }
    }
	count = sizeof(outbuf);
	if (count != (long)fwrite(outbuf, 1, count, f))
		fatal("write");
	close_output(f);
}

/* -------------------------------------------------------------------------- */

static struct font *read_eps_c16(const char *fname)
{
	struct font *p;
	FILE *f;
	char inbuf[256 * 16];
	int c, l;
	const char *basen;

	f = open_input(&fname, "rb");
	if (fread(inbuf, 1, sizeof(inbuf), f) != sizeof(inbuf))
		fatal("read");
	fclose(f);


	p = font_alloc();
	p->dat_table = (uint8_t *)xmalloc(sizeof(inbuf));
	p->off_table = (unsigned long *)xmalloc((256 + 1) * sizeof(*p->off_table));

	p->font_id = 999;
	p->point = 10;
	basen = strrchr(fname, '/');
	if (basen == NULL)
		basen = fname;
	else
		++basen;
	strncpy(p->name, basen, sizeof(p->name) - 1);
	p->first_ade = 0;
	p->last_ade = 255;
	p->top = 13;
	p->ascent = 11;
	p->half = 8;
	p->descent = 2;
	p->bottom = 2;
	p->max_char_width = 7;
	p->max_cell_width = 8;
	p->left_offset = 1;
	p->right_offset = 1;
	p->thicken = 1;
	p->ul_size = 1;
	p->lighten = 0x5555;
	p->skew = 0x5555;
	p->flags = F_MONOSPACE;
	for (c = 0; c <= 256; c++)
		p->off_table[c] = c * 8;
	p->form_width = 256;
	p->form_height = 16;

	for (c = 0; c < 256; c++)
	{
		for (l = 0; l < 16; l++)
		{
			p->dat_table[l * 256 + c] = inbuf[c * 16 + l];
		}
	}
	return p;
}

/* -------------------------------------------------------------------------- */

static struct font *read_stos_font(const char *fname)
{
	struct font *p;
	FILE *f;
	struct {
		unsigned char magic[4];
		unsigned char width[2];
		unsigned char height[2];
	} header;
	unsigned char maptab[256];
	char inbuf[256 * 16];
	int c, l;
	size_t offset;
	const char *basen;
	unsigned int char_width;
	size_t insize;
	
	f = open_input(&fname, "rb");
	if (fread(&header, 1, sizeof(header), f) != sizeof(header) ||
		fread(maptab, 1, sizeof(maptab), f) != sizeof(maptab))
		fatal("read");

	p = font_alloc();

	if (get_b_long(header.magic) != 0x06071963l ||
		(char_width = get_b_word(header.width)) != 1 ||
		((p->form_height = get_b_word(header.height)) < 6 || p->form_height > 16))
		fatal("unsupported format");
	insize = fread(inbuf, 1, p->form_height * 256, f);
	fclose(f);

	p->dat_table = (uint8_t *)xmalloc(sizeof(inbuf));
	p->off_table = (unsigned long *)xmalloc((256 + 1) * sizeof(*p->off_table));
	p->font_id = 999;
	p->point = p->form_height >= 16 ? 10 : 9;
	basen = strrchr(fname, '/');
	if (basen == NULL)
		basen = fname;
	else
		++basen;
	strncpy(p->name, basen, sizeof(p->name) - 1);
	p->first_ade = 0;
	p->last_ade = 255;
	p->top = p->form_height >= 16 ? 13 : 6;
	p->ascent = p->form_height >= 16 ? 11 : 6;
	p->half = p->form_height >= 16 ? 8 : 4;
	p->descent = p->form_height >= 16 ? 2 : 1;
	p->bottom = p->form_height >= 16 ? 2 : 1;
	p->max_char_width = 8;
	p->max_cell_width = 8;
	p->left_offset = 1;
	p->right_offset = 1;
	p->thicken = 1;
	p->ul_size = 1;
	p->lighten = 0x5555;
	p->skew = 0x5555;
	p->flags = F_MONOSPACE;
	for (c = 0; c <= 256; c++)
		p->off_table[c] = c * 8;
	p->form_width = 256;
	
	for (c = 0; c < 256; c++)
	{
		offset = maptab[c];
		offset = offset * char_width * p->form_height;
		if (offset + p->form_height > insize)
			fatal("offset %u for character %u out of range", maptab[c], c);
		for (l = 0; l < p->form_height; l++)
		{
			p->dat_table[l * 256 + c] = inbuf[offset + l];
		}
	}

	return p;
}

/* -------------------------------------------------------------------------- */

static void setpixel(unsigned char *bitmap, unsigned long offset, unsigned long form_width, int x, int y)
{
	unsigned char *pos;

	offset += x;
	pos = bitmap + y * form_width + (offset >> 3);
	*pos |= 0x80 >> (offset & 7);
}

/* -------------------------------------------------------------------------- */

static void draw_line(unsigned char *bitmap, unsigned long offset, unsigned long form_width, int x1, int y1, int x2, int y2)
{
	int dx, dy;
	int tmp;
	int yinc;
	int vec;
	int e1, e2, epsilon;

 	if (x1 == x2)
	{
		int y;

		if (y1 < y2)
		{
			for (y = y1; y <= y2; y++)
				setpixel(bitmap, offset, form_width, x1, y);
		} else
		{
			for (y = y1; y >= y2; y--)
				setpixel(bitmap, offset, form_width, x1, y);
		}
	} else if (y1 == y2)
	{
		int x;

		if (x1 < x2)
		{
			for (x = x1; x <= x2; x++)
				setpixel(bitmap, offset, form_width, x, y1);
		} else
		{
			for (x = x1; x >= x2; x--)
				setpixel(bitmap, offset, form_width, x, y1);
		}
	} else
	{
		if (x1 > x2)
		{
			tmp = x1;
			x1 = x2;
			x2 = tmp;
			tmp = y1;
			y1 = y2;
			y2 = tmp;
		}
		dx = x2 - x1;
		yinc = 1;
		dy = y2 - y1;
		if (dy < 0)
		{
			dy = -dy;
			yinc = -1;
		}
		vec = 0;
		if (dx >= dy)
		{
			tmp = dx;
			dx = dy;
			dy = tmp;
			vec = 1;
		}
		e1 = 2 * dx;
		epsilon = e1 - dy;
		e2 = epsilon - dy;
		
		if (vec == 0)
		{
			/* dY > dX */
			do
			{
				setpixel(bitmap, offset, form_width, x1, y1);
				y1 += yinc;
				if (epsilon >= 0)
				{
					x1++;
					epsilon += e2;
				} else
				{
					epsilon += e1;
				}
			} while (--dy >= 0);
		} else
		{
			/* dX > dY */
			do
			{
				setpixel(bitmap, offset, form_width, x1, y1);
				x1++;
				if (epsilon >= 0)
				{
					y1 += yinc;
					epsilon += e2;
				} else
				{
					epsilon += e1;
				}
			} while (--dy >= 0);
		}
	}
}

/* -------------------------------------------------------------------------- */

struct stroke {
	unsigned char opcode;
	int x;
	int y;
};

static struct font *read_bgi_font(const char *fname)
{
	struct font *p;
	FILE *f;
	struct {
		unsigned char magic[4];
		char copyright[86];
		unsigned char header_size[2];		/* Version 2.0 Header Format    */
		char font_name[4];					/* Font Internal Name       */
		unsigned char font_size[2];			/* Size in byte of file     */
		unsigned char font_major;			/* Driver Version Information   */
		unsigned char font_minor;
		unsigned char min_major;			/* BGI Revision Information */
		unsigned char min_minor;
		char filler[26];
		
		unsigned char sig;					/* SIGNATURE byte           */
		unsigned char nr_chars[2];			/* number of characters in file     */
		char mystery;						/* Currently Undefined          */
		unsigned char first_char;			/* first character in file */
		unsigned char cdefs[2];				/* offset to char definitions (relative to start of header) */
		unsigned char scan_flag;			/* True if set is scanable */
		signed char org_to_cap;				/* Height from origin to top of capitol */
		signed char org_to_base;			/* Height from origin to baseline   */
		signed char org_to_dec;				/* Height from origin to bot of decender */
		char fntname[4];					/* Four character name of font      */
		unsigned char unused;				/* Currently undefined          */
	} header;

	enum OP_CODES
	{
		END_OF_CHAR = 0,
		DO_SCAN = 1,
		MOVE = 2,
		DRAW = 3
	};

	unsigned int nr_chars;
	unsigned int last_chr;
	unsigned char offsetbuf[256 * 2];
	unsigned char widthbuf[256];
	struct {
		unsigned int offset;
		unsigned char width;
		int bitmap_width;
		int bitmap_height;
		int minx;
		int miny;
		int maxx;
		int maxy;
		unsigned int stroke_count;
		struct stroke *stroke;
	} charinfo[256];
	unsigned int i, j;
	long current, length;
	signed char *fontdata;
	unsigned long bitmap_offset;
	int minx, maxx, miny, maxy;
	int x, y, prevx, prevy;
	int xop, yop;

	f = open_input(&fname, "rb");
	if (fread(&header, 1, sizeof(header), f) != sizeof(header) ||
		get_b_long(header.magic) != 0x504b0808L ||
		(nr_chars = get_l_word(header.nr_chars)) == 0 ||
		(header.first_char + nr_chars) > 256)
		fatal("unsupported format");

	last_chr = header.first_char + nr_chars;

	memset(offsetbuf, 0, sizeof(offsetbuf));
	fread(&offsetbuf[header.first_char << 1], nr_chars, 2, f);
	memset(widthbuf, 0, sizeof(widthbuf));
	fread(&widthbuf[header.first_char], nr_chars, 1, f);
	memset(charinfo, 0, sizeof(charinfo));
	for (i = header.first_char; i < last_chr; i++)
	{
		charinfo[i].width = widthbuf[i];
		charinfo[i].offset = get_l_word(&offsetbuf[i << 1]);
	}
	
	current = ftell(f);
	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, current, SEEK_SET);
	length -= current;
	if (length <= 0)
	{
		fatal("corrupted font?");
	}
	fontdata = (signed char *)xmalloc(length);
	fread(fontdata, length, 1, f);
	fclose(f);
	
	for (i = header.first_char; i < last_chr; i++)
	{
		charinfo[i].width = widthbuf[i];
		charinfo[i].offset = get_l_word(&offsetbuf[i << 1]);
		if (charinfo[i].offset >= length)
		{
			fatal("corrupted font?");
		}
	}

	p = font_alloc();

	p->font_id = 999;
	p->point = 10;
	strncpy(p->name, header.font_name, 4);
	p->first_ade = header.first_char;
	p->last_ade = last_chr - 1;
	p->left_offset = 1;
	p->right_offset = 1;
	p->thicken = 1;
	p->ul_size = 1;
	p->lighten = 0x5555;
	p->skew = 0x5555;
	p->flags = 0;
	
	p->off_table = (unsigned long *)xmalloc((nr_chars + 1) * sizeof(*p->off_table));
	bitmap_offset = 0;
	minx = 0;
	maxx = -1;
	miny = 0;
	maxy = -1;
	for (i = 0; i < nr_chars; ++i)
	{
		unsigned int c = i + header.first_char;
		signed char *pb;
		unsigned int num_ops;
		int width, height;

		if (charinfo[c].offset == 0 && i != 0)
			continue;
		num_ops = 0;
		pb = fontdata + charinfo[c].offset;
		for (;;)
		{
			++num_ops;
			xop = *pb++;
			yop = *pb++;
			if (xop >= 0 && yop >= 0)
				break;
		}
		charinfo[c].stroke_count = num_ops;
		charinfo[c].stroke = (struct stroke *)xmalloc(num_ops * sizeof(*(charinfo[c].stroke)));
		pb = fontdata + charinfo[c].offset;
		x = y = prevx = prevy = 0;
		charinfo[c].maxx = -1;
		charinfo[c].maxy = -1;
		for (j = 0; j < num_ops; j++)
		{
			xop = *pb++;
			yop = *pb++;
			charinfo[c].stroke[j].x = xop & 0x7f;
			charinfo[c].stroke[j].y = yop & 0x7f;
			if (charinfo[c].stroke[j].x & 0x40)
				charinfo[c].stroke[j].x -= 128;
			if (charinfo[c].stroke[j].y & 0x40)
				charinfo[c].stroke[j].y -= 128;
			charinfo[c].stroke[j].opcode = 0;
			if (xop < 0)
				charinfo[c].stroke[j].opcode += 2;
			if (yop < 0)
				charinfo[c].stroke[j].opcode += 1;
			switch (charinfo[c].stroke[j].opcode)
			{
			case END_OF_CHAR:
			case DO_SCAN:
				break;
			case MOVE:
				xop = x + charinfo[c].stroke[j].x;
				yop = y + charinfo[c].stroke[j].y;
				prevx = xop;
				prevy = yop;
				break;
			case DRAW:
				xop = x + charinfo[c].stroke[j].x;
				yop = y + charinfo[c].stroke[j].y;
				if (prevx < charinfo[c].minx)
					charinfo[c].minx = prevx;
				if (prevx > charinfo[c].maxx)
					charinfo[c].maxx = prevx;
				if (prevy < charinfo[c].miny)
					charinfo[c].miny = prevy;
				if (prevy > charinfo[c].maxy)
					charinfo[c].maxy = prevy;
				if (xop < charinfo[c].minx)
					charinfo[c].minx = xop;
				if (xop > charinfo[c].maxx)
					charinfo[c].maxx = xop;
				if (yop < charinfo[c].miny)
					charinfo[c].miny = yop;
				if (yop > charinfo[c].maxy)
					charinfo[c].maxy = yop;
				prevx = xop;
				prevy = yop;
				break;
			}
		}
		width = charinfo[c].maxx - charinfo[c].minx + 1;
		height = charinfo[c].maxy - charinfo[c].miny + 1;
		p->off_table[i] = bitmap_offset;
		if (width == 0)
			width = charinfo[c].width;
		bitmap_offset += width;
		charinfo[c].bitmap_width = width;
		charinfo[c].bitmap_height = height;
		if (charinfo[c].minx < minx)
			minx = charinfo[c].minx;
		if (charinfo[c].maxx > maxx)
			maxx = charinfo[c].maxx;
		if (charinfo[c].miny < miny)
			miny = charinfo[c].miny;
		if (charinfo[c].maxy > maxy)
			maxy = charinfo[c].maxy;
	}
	p->off_table[i] = bitmap_offset;

	p->form_width = bitmap_offset;
	p->form_width = ((p->form_width + 15) >> 4) << 1;
	
	p->max_char_width = maxx - minx + 1;
	p->max_cell_width = p->max_char_width;
	p->form_height = maxy - miny + 1;

	p->top = p->form_height - 1;
	p->ascent = header.org_to_cap - header.org_to_dec;
	p->bottom = -header.org_to_dec;
	p->descent = p->bottom;
	p->half = -header.org_to_dec + header.org_to_cap / 2;

	p->dat_table = (uint8_t *)xmalloc((size_t)p->form_width * p->form_height);
	
	for (i = 0; i < nr_chars; i++)
	{
		unsigned int c = i + header.first_char;

		if (charinfo[c].offset == 0 && i != 0)
		{
			p->off_table[i] = F_NO_CHARL;
			continue;
		}
		
		x = -charinfo[c].minx;
		y = -miny;
		prevx = x;
		prevy = y;
#if 0
		fprintf(stderr, "Char $%02x (%c)    Offset: $%04x   Width: %-5d   Stroke Count: %d   Bounding = %d %d %d %d\n",
			c, isprint(c) ? c : '.',
			charinfo[c].offset, charinfo[c].width, charinfo[c].stroke_count,
			charinfo[c].minx, charinfo[c].maxx, charinfo[c].miny, charinfo[c].maxy);
#endif

		for (j = 0; j < charinfo[c].stroke_count; j++)
		{

#if 0
			static const char *const OpName[] = {
				"End    ",
				"Do Scan",
				"Move To",
				"Line To"
			};

			fprintf(stderr, "  %3d : OpCode: %s (%d)   X: %4d   Y: %4d\n", j, OpName[charinfo[c].stroke[j].opcode], charinfo[c].stroke[j].opcode, charinfo[c].stroke[j].x, charinfo[c].stroke[j].y);
#endif

			switch (charinfo[c].stroke[j].opcode)
			{
			case END_OF_CHAR:
			case DO_SCAN:
				break;
			case MOVE:
				xop = x + charinfo[c].stroke[j].x;
				yop = y + charinfo[c].stroke[j].y;
				prevx = xop;
				prevy = yop;
				break;
			case DRAW:
				xop = x + charinfo[c].stroke[j].x;
				yop = y + charinfo[c].stroke[j].y;
				draw_line(p->dat_table, p->off_table[i], p->form_width, prevx, p->form_height - prevy - 1, xop, p->form_height - yop - 1);
				prevx = xop;
				prevy = yop;
				break;
			}
		}
	}

	for (i = 0; i < nr_chars; i++)
		free(charinfo[i].stroke);
	free(fontdata);

	return p;
}

/* ************************************************************************** */
/* -------------------------------------------------------------------------- */
/* ************************************************************************** */

static int file_type(const char *c)
{
	int n;
	const char *ext;
	
	if (c == NULL || strcmp(c, "-") == 0)
		return FILE_TXT;
	n = (int)strlen(c);
	if (n >= 3 && c[n - 2] == '.' && (c[n - 1] == 'c' || c[n - 1] == 'C' || c[n - 1] == 'h' || c[n - 1] == 'H'))
		return FILE_C;
	if (n < 5 || c[n - 4] != '.')
		return 0;
	ext = c + n - 3;
	if (strcmp(ext, "txt") == 0 || strcmp(ext, "TXT") == 0)
		return FILE_TXT;
	if (strcmp(ext, "fnt") == 0 || strcmp(ext, "FNT") == 0)
		return FILE_FNT;
	if (strcmp(ext, "bmp") == 0 || strcmp(ext, "BMP") == 0)
		return FILE_BMP;
	if (strcmp(ext, "c16") == 0 || strcmp(ext, "C16") == 0)
		return FILE_C16;
	if (strcmp(ext, "cr0") == 0 || strcmp(ext, "CR0") == 0 ||
		strcmp(ext, "cr1") == 0 || strcmp(ext, "CR1") == 0 ||
		strcmp(ext, "cr2") == 0 || strcmp(ext, "CR2") == 0 ||
		strcmp(ext, "cr3") == 0 || strcmp(ext, "CR3") == 0)
		return FILE_CRX;
	if (strcmp(ext, "chr") == 0 || strcmp(ext, "CHR") == 0)
		return FILE_BGI;
	if (strcmp(ext, "ttf") == 0 || strcmp(ext, "TTF") == 0)
		return FILE_TTF;
	return 0;
}

/* -------------------------------------------------------------------------- */

enum opt {
	OPTION_FLAG_SET = 0,
	OPTION_ERROR = '?',
	OPTION_OUTPUT = 'o',
	OPTION_ARANYM = 'a',
	OPTION_PLAIN = 'p',
	OPTION_ALLCHARS = 'A',
	OPTION_NO_OFFTABLE = 'O',
	OPTION_VARNAME = 'v',
	OPTION_FONTNAME = 'n',
	OPTION_SCALE = 's',
	OPTION_GRID = 'g',
	
	OPTION_HELP = 'h',
	OPTION_VERSION = 'V'
};


static struct option const long_options[] = {
	{ "aranym", no_argument, NULL, OPTION_ARANYM },
	{ "plain", no_argument, NULL, OPTION_PLAIN },
	{ "all-chars", no_argument, NULL, OPTION_ALLCHARS },
	{ "output", required_argument, NULL, OPTION_OUTPUT },
	{ "varname", required_argument, NULL, OPTION_VARNAME },
	{ "name", required_argument, NULL, OPTION_FONTNAME },
	{ "no-offtable", no_argument, NULL, OPTION_NO_OFFTABLE },
	{ "scale", required_argument, NULL, OPTION_SCALE },
	{ "grid", required_argument, NULL, OPTION_GRID },
	{ "help", no_argument, NULL, OPTION_HELP },
	{ "version", no_argument, NULL, OPTION_VERSION },
	{ NULL, no_argument, NULL, 0 }
};

/* -------------------------------------------------------------------------- */

static void print_version(void)
{
}

/* -------------------------------------------------------------------------- */

static void usage(FILE *f, int errcode)
{
	fprintf(f, "\
Usage: \n\
  fntconv -o <to> <from>\n\
    converts BDOS font between types C, TXT, FNT.\n\
    The file types are inferred from the file extensions\n\
    (not all combinations are allowed.)\n\
Options:\n\
  -o, --output <file>   write output to <file>\n\
  -a, --aranym          output C sourcecode for ARAnyM\n\
  -p, --plain           output C sourcecode for TOS\n\
  -A, --all-chars       output data also for non-existent chars\n\
  -v, --varname <name>  set the name of the font header variable\n\
  -n, --name <name>     set the name of the font\n\
  -O, --no-offtable     do not write the offsets table\n\
  -s, --scale <factor>  scale picture up (BMP only)\n\
  -g, --grid <width>    draw grid around characters (BMP only)\n\
Supported formats for reading:\n\
  .txt, .fnt, .c16, .cr0, .chr\n\
Supported formats for writing:\n\
  .c, .txt, .fnt, .c16, .bmp, .ttf\n\
");
	exit(errcode);
}

/* -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
	struct font *p;
	const char *from = NULL;
	const char *to = NULL;
	int c;
	
	while ((c = getopt_long_only(argc, argv, "o:ag:s:v:n:pAOhV", long_options, NULL)) != EOF)
	{
		const char *arg = optarg;
		switch ((enum opt) c)
		{
		case OPTION_OUTPUT:
			to = arg;
			break;
		case OPTION_ARANYM:
			for_aranym = 1;
			all_chars = 1;
			break;
		case OPTION_PLAIN:
			for_plain = 1;
			break;
		case OPTION_ALLCHARS:
			all_chars = 1;
			break;
		case OPTION_NO_OFFTABLE:
			do_off_table = 0;
			break;
		case OPTION_VARNAME:
			varname = optarg;
			break;
		case OPTION_FONTNAME:
			g_fontname = optarg;
			break;
		case OPTION_SCALE:
			scale = (int)strtol(optarg, NULL, 0);
			if (scale <= 0 || scale > 100)
				usage(stderr, EXIT_FAILURE);
			break;
		case OPTION_GRID:
			grid = (int)strtol(optarg, NULL, 0);
			if (grid < 0 || grid > 100)
				usage(stderr, EXIT_FAILURE);
			break;
		
		case OPTION_VERSION:
			print_version();
			return EXIT_SUCCESS;
			
		case OPTION_HELP:
			usage(stdout, EXIT_SUCCESS);
			break;
		
		/* returned when only flag was set */
		case OPTION_FLAG_SET:
			break;
		
		/* returned for unknown/invalid option */
		case OPTION_ERROR:
			exit(EXIT_FAILURE);
			break;
		}
	}
	
	if ((argc - optind) != 1)
		usage(stderr, EXIT_FAILURE);

	from = argv[optind];
	
	convert_from = file_type(from);
	convert_to = file_type(to);
	
	switch (convert_from)
	{
	case FILE_C:
		fatal("cannot read C files");
		return EXIT_FAILURE;
	case FILE_TXT:
		p = read_txt(from);
		break;
	case FILE_FNT:
		p = read_fnt(from);
		break;
	case FILE_BMP:
		fatal("cannot read BMP files");
		return EXIT_FAILURE;
	case FILE_C16:
		p = read_eps_c16(from);
		break;
	case FILE_CRX:
		p = read_stos_font(from);
		break;
	case FILE_BGI:
		p = read_bgi_font(from);
		break;
	case FILE_TTF:
		fatal("cannot read TTF files");
		return EXIT_FAILURE;
	default:
		fatal("wrong file type");
		return EXIT_FAILURE;
	}

	switch (convert_to)
	{
	case FILE_C:
		if (for_aranym)
			write_c_aranym(p, to);
		else if (for_plain)
			write_c_emutos(p, to, 1);
		else
			write_c_emutos(p, to, 0);
		break;
	case FILE_TXT:
		write_txt(p, to);
		break;
	case FILE_FNT:
		write_fnt(p, to);
		break;
	case FILE_BMP:
		write_bmp(p, to);
		break;
	case FILE_C16:
		write_eps_c16(p, to);
		break;
	case FILE_TTF:
		write_ttf(p, to);
		break;
	case FILE_CRX:
	case FILE_BGI:
	default:
		fatal("wrong file type");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
