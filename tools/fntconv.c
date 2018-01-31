/*
 * fntconv.c : convert Gemdos fonts between FNT, TXT and C formats
 *
 * Copyright (c) 2001 Laurent Vogel
 *
 * Authors:
 *  LVL     Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef unsigned char UBYTE;
typedef signed char BYTE;
typedef unsigned short UWORD;
typedef short WORD;
typedef unsigned long ULONG;
typedef long LONG;

#define MAX_ADE 0x7fff

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


/*
 * memory
 */
static void *xmalloc(size_t s)
{
	void *a = calloc(1, s);

	if (a == 0)
		fatal("memory");
	return a;
}

/*
 * xstrdup
 */

static char *xstrdup(const char *s)
{
	int len = strlen(s);
	char *a = xmalloc(len + 1);

	strcpy(a, s);
	return a;
}


/*
 * little/big endian conversion
 */

static LONG get_b_long(void *addr)
{
	UBYTE *uaddr = (UBYTE *) addr;

	return (uaddr[0] << 24) + (uaddr[1] << 16) + (uaddr[2] << 8) + uaddr[3];
}


static void set_b_long(void *addr, LONG value)
{
	UBYTE *uaddr = (UBYTE *) addr;

	uaddr[0] = value >> 24;
	uaddr[1] = value >> 16;
	uaddr[2] = value >> 8;
	uaddr[3] = value;
}


static WORD get_b_word(void *addr)
{
	UBYTE *uaddr = (UBYTE *) addr;

	return (uaddr[0] << 8) + uaddr[1];
}


static void set_b_word(void *addr, WORD value)
{
	UBYTE *uaddr = (UBYTE *) addr;

	uaddr[0] = value >> 8;
	uaddr[1] = value;
}

static LONG get_l_long(void *addr)
{
	UBYTE *uaddr = (UBYTE *) addr;

	return (uaddr[3] << 24) + (uaddr[2] << 16) + (uaddr[1] << 8) + uaddr[0];
}


#if 0
static void set_l_long(void *addr, LONG value)
{
	UBYTE *uaddr = (UBYTE *) addr;

	uaddr[3] = value >> 24;
	uaddr[2] = value >> 16;
	uaddr[1] = value >> 8;
	uaddr[0] = value;
}
#endif


static WORD get_l_word(void *addr)
{
	UBYTE *uaddr = (UBYTE *) addr;

	return (uaddr[1] << 8) + uaddr[0];
}


#if 0
static void set_l_word(void *addr, WORD value)
{
	UBYTE *uaddr = (UBYTE *) addr;

	uaddr[1] = value >> 8;
	uaddr[0] = value;
}
#endif


/*
 * fontdef.h - font-header definitions
 *
 */

/* fh_flags   */

#define F_DEFAULT    1					/* this is the default font (face and size) */
#define F_HORZ_OFF   2					/* there are left and right offset tables */
#define F_STDFORM    4					/* is the font in standard format */
#define F_MONOSPACE  8					/* is the font monospaced */
#define F_EXT_HDR    32					/* extended header */

#define F_EXT_HDR_SIZE 62				/* size of an additional extended header */

/* style bits */

#define F_THICKEN 1
#define F_LIGHT	2
#define F_SKEW	4
#define F_UNDER	8
#define F_OUTLINE 16
#define F_SHADOW	32

struct font_file_hdr
{										/* describes a .FNT file header */
	UBYTE font_id[2];
	UBYTE point[2];
	char name[32];
	UBYTE first_ade[2];
	UBYTE last_ade[2];
	UBYTE top[2];
	UBYTE ascent[2];
	UBYTE half[2];
	UBYTE descent[2];
	UBYTE bottom[2];
	UBYTE max_char_width[2];
	UBYTE max_cell_width[2];
	UBYTE left_offset[2];				/* amount character slants left when skewed */
	UBYTE right_offset[2];				/* amount character slants right */
	UBYTE thicken[2];					/* number of pixels to smear */
	UBYTE ul_size[2];					/* size of the underline */
	UBYTE lighten[2];					/* mask to and with to lighten  */
	UBYTE skew[2];						/* mask for skewing */
	UBYTE flags[2];

	UBYTE hor_table[4];					/* offset of horizontal offsets */
	UBYTE off_table[4];					/* offset of character offsets  */
	UBYTE dat_table[4];					/* offset of character definitions */
	UBYTE form_width[2];
	UBYTE form_height[2];
	UBYTE next_font[4];
};

struct font
{										/* describes a font in memory */
	WORD font_id;
	WORD point;
	char name[32];
	UWORD first_ade;
	UWORD last_ade;
	UWORD top;
	UWORD ascent;
	UWORD half;
	UWORD descent;
	UWORD bottom;
	UWORD max_char_width;
	UWORD max_cell_width;
	UWORD left_offset;					/* amount character slants left when skewed */
	UWORD right_offset;					/* amount character slants right */
	UWORD thicken;						/* number of pixels to smear */
	UWORD ul_size;						/* size of the underline */
	UWORD lighten;						/* mask to and with to lighten  */
	UWORD skew;							/* mask for skewing */
	UWORD flags;

	UBYTE *hor_table;					/* horizontal offsets */
	UWORD *off_table;					/* character offsets, 0xFFFF if no char present.  */
	UBYTE *dat_table;					/* character definitions */
	UWORD form_width;
	UWORD form_height;
};

#define F_NO_CHAR 0xFFFFu



/*
 * text input files
 */

#define BACKSIZ 10
#define READSIZ 512

typedef struct ifile
{
	int lineno;
	char *fname;
	FILE *fh;
	UBYTE buf[BACKSIZ + READSIZ];
	int size;
	int index;
	int ateof;
} IFILE;

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


static IFILE *ifopen(const char *fname)
{
	IFILE *f = xmalloc(sizeof(IFILE));

	f->fname = xstrdup(fname);
	f->fh = fopen(fname, "rb");
	if (f->fh == 0)
	{
		free(f);
		return NULL;
	}
	f->size = 0;
	f->index = 0;
	f->ateof = 0;
	f->lineno = 1;
	return f;
}

static void ifclose(IFILE *f)
{
	fclose(f->fh);
	free(f);
}


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
			fprintf(stderr, "file %s, line %d too long\n", f->fname, f->lineno);
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


static int try_given_string(char **cc, char *s)
{
	int n = strlen(s);

	if (!strncmp(*cc, s, n))
	{
		*cc += n;
		return 1;
	} else
	{
		return 0;
	}
}


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


static int try_eol(char **cc)
{
	return (**cc == 0) ? 1 : 0;
}


/*
 * simple bitmap read/write
 */

static int get_bit(UBYTE *addr, int i)
{
	return (addr[i / 8] & (1 << (7 - (i & 7)))) ? 1 : 0;
}

static void set_bit(UBYTE *addr, int i)
{
	addr[i / 8] |= (1 << (7 - (i & 7)));
}

/*
 * read functions
 */

static struct font *read_txt(const char *fname)
{
	IFILE *f;
	int ch, i, j, k;
	long off;
	int height;
	int width = 0;
	int w;
	UBYTE *bms;
	int bmsize, bmnum;
	int first, last;
	UBYTE *b;
	char *c;
	long u;
	char line[200];
	struct font *p;
	int max = 80;

	p = xmalloc(sizeof(*p));
	if (p == NULL)
		fatal("memory");
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
	if (first < 0 || last < 0 || first > MAX_ADE || first > last)
	{
		fatal("wrong char range : first = %d, last = %d", first, last);
	}
	if (p->max_cell_width >= 40 || p->form_height >= 40)
	{
		fatal("unreasonable font size %dx%d", p->max_cell_width, height);
	}

	/* allocate a big buffer to hold all bitmaps */
	bmnum = last - first + 1;
	bmsize = (p->max_cell_width * height + 7) >> 3;
	bms = xmalloc((size_t)bmsize * bmnum);
	p->off_table = xmalloc(sizeof(*p->off_table) * (bmnum + 1));
	for (i = 0; i < bmnum; i++)
	{
		p->off_table[i] = F_NO_CHAR;
	}

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
		ch = u;
		if (ch < first || ch > last)
		{
			fprintf(stderr, "wrong character number %u\n", ch);
			goto fail;
		}

		ch -= first;
		if (p->off_table[ch] != F_NO_CHAR)
		{
			fprintf(stderr, "character number %u was already defined\n", ch + first);
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
				if (*c == 'X')
				{
					set_bit(b, k);
				} else if (*c == '.')
				{
				} else {
					fprintf(stderr, "illegal character '%c' in bitmap definition\n", *c);
					goto fail;
				}
				k++;
			}
			if (i == 0)
			{
				width = w;
			} else if (w != width)
			{
				fprintf(stderr, "bitmap lines of different lengths\n");
				goto fail;
			}
		}
		EXPECT("endchar");
		p->off_table[ch] = width;			/* != F_NO_CHAR, real value filled later */
	}
	ifclose(f);
#undef EXPECT

	/* compute size of final form, and compute offs from widths */
	off = 0;
	for (i = 0; i < bmnum; i++)
	{
		width = p->off_table[i];
		p->off_table[i] = (UWORD)off;
		if (width != F_NO_CHAR)
			off += width;
	}
	p->off_table[bmnum] = (UWORD)off;
	if (off >= 0x10000L)
	{
		fprintf(stderr, "font width exceeded\n");
		goto fail;
	}
	p->form_width = ((off + 15) >> 4) << 1;
	p->dat_table = xmalloc((size_t)height * p->form_width);

	/* now, pack the bitmaps in the destination form */
	for (i = 0; i < bmnum; i++)
	{
		off = p->off_table[i];
		width = p->off_table[i + 1] - off;
		b = bms + bmsize * i;
		k = 0;
		for (j = 0; j < height; j++)
		{
			for (w = 0; w < width; w++)
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
	fprintf(stderr, "fatal error file %s line %d\n", f->fname, f->lineno - 1);
	ifclose(f);
	exit(EXIT_FAILURE);
}


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
	
	p = malloc(sizeof(struct font));
	if (p == NULL)
		fatal("memory");
	f = fopen(fname, "rb");
	if (f == NULL)
		fatal("fopen");

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
	if (p->flags & F_HORZ_OFF)
	{
		fatal("horizontal offsets not handled");
	}
	if (p->last_ade > MAX_ADE || p->first_ade >= p->last_ade)
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
	p->off_table = xmalloc(sizeof(*p->off_table) * (bmnum + 1));
	
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
		}
	}
	if (fseek(f, off_dat_table, SEEK_SET))
		fatal("seek");
	count = p->form_height * p->form_width;
	p->dat_table = malloc(count);
	if (p->dat_table == NULL)
		fatal("memory");
	if ((long)fread(p->dat_table, 1, count, f) != count)
		fatal("short read");

#if 0
	if (off_hor_table)
	{
		p->flags |= F_HORZ_OFF;
	}
#endif
	if (p->flags & F_HORZ_OFF)
	{
		UBYTE buf[2];
		int i;

		p->hor_table = xmalloc(bmnum * 2);
		if (fseek(f, off_hor_table, SEEK_SET))
			fatal("seek");
		for (i = 0; i < bmnum; i++)
		{
			count = 2;
			if ((long)fread(buf, 1, count, f) != count)
				fatal("short read");
			p->hor_table[2 * i] = buf[0];
			p->hor_table[2 * i + 1] = buf[1];
		}
	}
	fclose(f);
	return p;
}


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
	
	f = fopen(fname, "wb");
	if (f == NULL)
		fatal("fopen");

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

	bmnum = p->last_ade - p->first_ade + 1;
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
		fatal("TODO");
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
	if (fclose(f))
		fatal("fclose");
}


static void write_txt(struct font *p, const char *filename)
{
	FILE *f;
	int i;

	/* first, write header */

	f = fopen(filename, "w");
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
		int r, c, w, off;

		c = i - p->first_ade;
		off = p->off_table[c];
		if (off == F_NO_CHAR)
			continue;
		w = p->off_table[c + 1] - off;
		if ((off + w) > (8 * p->form_width))
		{
			fprintf(stderr, "char %d: offset %d + width %d out of range (%d)\n", i, off, w, 8 * p->form_width);
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
	fclose(f);
}


static void write_c(struct font *p, const char *filename)
{
	FILE *f;
	int i;
	int bmnum;
	
	/* first, write header */

	f = fopen(filename, "w");
	fprintf(f, "\
/*\n\
 * %s - a font in standard format\n\
 *\n\
 * Automatically generated by fntconv.c\n\
 */\n", filename);
	fprintf(f, "\
\n\
#include \"portab.h\"\n\
#include \"fonthdr.h\"\n\
\n");
	fprintf(f, "\
static UWORD off_table[], dat_table[];\n\
\n");
	fprintf(f, "struct font_head THISFONT = {\n");


#define SET_WORD(a) fprintf(f, "    %u,  /* " #a " */\n", p->a)
	SET_WORD(font_id);
	SET_WORD(point);

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
	fprintf(f, "\",  /*   BYTE name[32]	*/\n");

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

	fprintf(f, "    0x%04x, /* lighten */\n", p->lighten);
	fprintf(f, "    0x%04x, /* skew */\n", p->skew);
	/* TODO */
	fprintf(f, "    F_STDFORM | F_MONOSPACE | F_DEFAULT,  /* flags */\n");
	fprintf(f, "    0,			/*   UBYTE *hor_table	*/\n");
	fprintf(f, "    off_table,		/*   UWORD *off_table	*/\n");
	fprintf(f, "    dat_table,		/*   UBYTE *dat_table	*/\n");

	SET_WORD(form_width);
	SET_WORD(form_height);
#undef SET_WORD
	fprintf(f, "    0,  /* struct font * next_font */\n");
	fprintf(f, "    0   /* UWORD next_seg */\n};\n\n");

	bmnum = p->last_ade - p->first_ade + 1;
	fprintf(f, "static UWORD off_table[] =\n{\n");
	{
		for (i = 0; i <= bmnum; i++)
		{
			if ((i & 7) == 0)
				fprintf(f, "    ");
			else
				fprintf(f, " ");
			fprintf(f, "0x%04x", p->off_table[i]);
			if (i != bmnum)
				fprintf(f, ",");
			if ((i & 7) == 7)
				fprintf(f, "\n");
		}
	}
	fprintf(f, "\n};\n\n");

	fprintf(f, "static UWORD dat_table[] =\n{\n");
	{
		int h;
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

	fclose(f);
}

#define FILE_C 1
#define FILE_TXT 2
#define FILE_FNT 3

static int file_type(const char *c)
{
	int n = strlen(c);

	if (n >= 3 && c[n - 2] == '.' && (c[n - 1] == 'c' || c[n - 1] == 'C' || c[n - 1] == 'h' || c[n - 1] == 'H'))
		return FILE_C;
	if (n < 5 || c[n - 4] != '.')
		return 0;
	if (strcmp(c + n - 3, "txt") == 0 || strcmp(c + n - 3, "TXT") == 0)
		return FILE_TXT;
	if (strcmp(c + n - 3, "fnt") == 0 || strcmp(c + n - 3, "FNT") == 0)
		return FILE_FNT;
	return 0;
}

int main(int argc, char **argv)
{
	struct font *p;
	char *from,	*to;

	if (argc != 4)
		goto usage;
	if (strcmp(argv[1], "-o") == 0)
	{
		from = argv[3];
		to = argv[2];
	} else if (strcmp(argv[2], "-o") == 0)
	{
		from = argv[1];
		to = argv[3];
	} else
	{
		goto usage;
	}
	
	switch (file_type(from))
	{
	case FILE_C:
		fatal("cannot read C files");
		return 1;
	case FILE_TXT:
		p = read_txt(from);
		break;
	case FILE_FNT:
		p = read_fnt(from);
		break;
	default:
		fatal("wrong file type");
		return EXIT_FAILURE;
	}
	switch (file_type(to))
	{
	case FILE_C:
		write_c(p, to);
		break;
	case FILE_TXT:
		write_txt(p, to);
		break;
	case FILE_FNT:
		write_fnt(p, to);
		break;
	default:
		fatal("wrong file type");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

  usage:
	fprintf(stderr, "\
Usage: \n\
  fntconv <from> -o <to>\n\
  fntconv -o <to> <from>\n\
    converts BDOS font between types C, TXT, FNT.\n\
    the file types are inferred from the file extensions.\n\
    (not all combinations are allowed.)\n");
	
	return EXIT_FAILURE;
}
