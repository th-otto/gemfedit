/* fnttobdf
 * This utility converts GEM Bitmap fonts (.FNT) to BDF files (.bdf), which
 * can then be converted to .pcf files using "bdftopcf".
 * It can also convert the fonts to ISO-Latin1 encoding.
 *
 * Written by Thorsten Otto (June, 2013)
 */
#include "linux/libcwrap.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <endian.h>
#include "fonthdr.h"

typedef int8_t B;
typedef uint8_t UB;
typedef int16_t W;
typedef uint16_t UW;
typedef int32_t L;
typedef uint32_t UL;

typedef int gboolean;
#ifndef FALSE
#  define FALSE 0
#  define TRUE  1
#endif
#define UNUSED(x) (void)(x)

static char const program_name[] = "fnt2bdf";
static gboolean translate = FALSE;
static gboolean info = FALSE;
static gboolean verbose = FALSE;
static gboolean quiet = FALSE;
static gboolean nocomment = FALSE;
static const char *foundry = "atari";


/*
 * used to include "fonthdr.h" here, but we can't trust non-TOS compilers
 * about their alignment requirements
 */

#ifndef EXIT_FAILURE
#  define EXIT_FAILURE 1
#  define EXIT_SUCCESS 0
#endif

static inline B *TO_B(void *s) { return (B *)s; }
static inline UB *TO_UB(void *s) { return (UB *)s; }
static inline W *TO_W(void *s) { return (W *)s; }
static inline UW *TO_UW(void *s) { return (UW *)s; }
static inline L *TO_L(void *s) { return (L *)s; }
static inline UL *TO_UL(void *s) { return (UL *)s; }

/* Load/Store primitives (without address checking) */
#define LOAD_B(_s) (*(TO_B(_s)))
#define LOAD_UB(_s) (*(TO_UB(_s)))
#define LOAD_W(_s) (*(TO_W(_s)))
#define LOAD_UW(_s) (*(TO_UW(_s)))
#define LOAD_L(_s) (*(TO_L(_s)))
#define LOAD_UL(_s) (*(TO_UL(_s)))

#define STORE_B(_d,_v) *(TO_B(_d)) = _v
#define STORE_UB(_d,_v)	*(TO_UB(_d)) = _v
#define STORE_W(_d,_v) *(TO_W(_d)) = _v
#define STORE_UW(_d,_v) *(TO_UW(_d)) = _v
#define STORE_L(_d,_v) *(TO_L(_d)) = _v
#define STORE_UL(_d,_v) *(TO_UL(_d)) = _v

#define LM_B LOAD_B
#define LM_UB LOAD_UB
#define SM_B STORE_B
#define SM_UB STORE_UB

static inline UW swap_w(UW x)
{
	return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

static inline UL swap_l(UL x)
{
	return ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));
}

#define SWAP_W(s) STORE_UW(s, ((UW)swap_w(LOAD_UW(s))))
#define SWAP_L(s) STORE_UL(s, ((UL)swap_l(LOAD_UL(s))))

static UB *m;
static unsigned short tr[256];
static unsigned char invtr[0x10000];


#define WEIGHT  "medium"
#define SLANT   "r"


static char *font_name(const char *n, int p, int w, int h, int f, int i)
{
	static char s[200];

	UNUSED(w);
	UNUSED(h);
	if (translate)
	{
		sprintf(s, "-%s-%s-%s-%s-normal--%d-%d-100-100-%c-100-iso8859-1", foundry, n, WEIGHT, SLANT, p, 10 * p, (f & FONTF_MONOSPACED) ? 'm' : 'p');
	} else
	{
		sprintf(s, "%s-%d-%d", n, p, i);
	}
	return s;
}


static void print_line(UB *dat, int off, int w, FILE *out)
{
	UL x;
	int j, b;
	UB inmask;
	UB outmask;
	int p;
	
	b = (off & 7);
	inmask = 0x80 >> b;
	p = off >> 3;
	outmask = 0x80;
	j = 0;
	x = 0;
	while (w > 0)
	{
#if 0
		fprintf(stderr, "off=%d, w=%d, j=%d, p=%d, x=%ld b=%d\n", off, w, j, p, x, b);
#endif
		if (dat[p] & inmask)
			x |= outmask;
		inmask >>= 1;
		b++;
		if (b == 8)
		{
			p++;
			inmask = 0x80;
			b = 0;
		}
		outmask >>= 1;
		j++;
		if (j == 8)
		{
			fprintf(out, "%02x", x);
			j = 0;
			x = 0;
			outmask = 0x80;
		}
		w--;
	}
	if (j > 0)
		fprintf(out, "%02x", x);
	fprintf(out, "\n");
}


static void print_comment(UB *dat, int off, int w, FILE *out)
{
	int b;
	UB inmask;
	int p;
	
	fprintf(out, "COMMENT ");
	b = (off & 7);
	inmask = 0x80 >> b;
	p = off >> 3;
	while (w > 0)
	{
		fputc(dat[p] & inmask ? '*' : ' ', out);
		inmask >>= 1;
		b++;
		if (b == 8)
		{
			p++;
			inmask = 0x80;
			b = 0;
		}
		w--;
	}
	fprintf(out, "\n");
}


static void swap_gemfnt_header(UB *h, unsigned int l)
{
	UB *u;
	
	if (l < 84)
		return;
	SWAP_W(h + 0); /* font_id */
	SWAP_W(h + 2); /* point */
	/* skip name */
	for (u = h + 36; u < h + 68; u += 2) /* first_ade .. flags */
	{
		SWAP_W(u);
	}
	SWAP_L(h + 68); /* hor_table */
	SWAP_L(h + 72); /* off_table */
	SWAP_L(h + 76); /* dat_table */
	SWAP_W(h + 80); /* form_width */
	SWAP_W(h + 82); /* form_height */
}


/*
 * There are apparantly several fonts that have the Motorola flag set
 * but are stored in little-endian format.
 */
static gboolean check_gemfnt_header(UB *h, unsigned int l)
{
	UW firstc, lastc, points;
	UW form_width, form_height;
	UW cellwidth;
	UL dat_offset;
	UL off_table;
	
	if (l < 84)
		return FALSE;
	firstc = LOAD_UW(h + 36);
	lastc = LOAD_UW(h + 38);
	points = LOAD_UW(h + 2);
	if (lastc == 256)
	{
		lastc = 255;
	}
	if (firstc >= 0x2000 || lastc >= 0xff00 || firstc > lastc)
		return FALSE;
	if (points >= 0x300)
		return FALSE;
	if (LOAD_UL(h + 68) >= l)
		return FALSE;
	off_table = LOAD_UL(h + 72);
	if (off_table < 84 || (off_table + (lastc - firstc + 1) * 2) > l)
		return FALSE;
	dat_offset = LOAD_UL(h + 76);
	if (dat_offset < 84 || dat_offset >= l)
		return FALSE;
	cellwidth = LOAD_UW(h + 52);
	if (cellwidth == 0)
		return FALSE;
	form_width = LOAD_UW(h + 80);
	form_height = LOAD_UW(h + 82);
	if ((dat_offset + form_width * form_height) > l)
		return FALSE;
	STORE_UW(h + 38, lastc);
	return TRUE;
}


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


static void fix_name(char *name)
{
	int len = strlen(name);
	
	while (len > 0)
	{
		while (len > 0 && name[len - 1] == ' ')
			name[--len] = '\0';
		if (len > 0 && isdigit(name[len - 1]))
		{
			while (len > 0 && isdigit(name[len - 1]))
				name[--len] = '\0';
			if (len > 0 && name[len - 1] == 'x')
			{
				name[--len] = '\0';
				while (len > 0 && isdigit(name[len - 1]))
					name[--len] = '\0';
			}
			continue;
		}
		if (len > 4 && strcasecmp(name + len - 4, "bold") == 0)
		{
			len -= 4;
			name[len] = '\0';
			continue;
		}
		if (len > 5 && strcasecmp(name + len - 5, "punkt") == 0)
		{
			len -= 5;
			name[len] = '\0';
			continue;
		}
		if (len > 9 && strcasecmp(name + len - 9, "monospace") == 0)
		{
			len -= 9;
			name[len] = '\0';
			continue;
		}
		if (len > 12 && strcasecmp(name + len - 12, "proportional") == 0)
		{
			len -= 12;
			name[len] = '\0';
			continue;
		}
		return;
	}
}


static void fnttobdf(UB *b, int l, FILE *out, const char *filename)
{
	UB *hor_table;
	UB *off_table;
	UB *dat_table;
	int i;
	UB *h = b;
	int numoffs;
	UW flags;
	UW mono;
	UW form_width, form_height;
	UW firstc, lastc;
	W top;
	W max_cell_width;
	char facename_buf[VDI_FONTNAMESIZE];
	char family_name[VDI_FONTNAMESIZE];
	int default_char;
	
#if BYTE_ORDER == BIG_ENDIAN
#  define HOST_BIG 1
#else
#  define HOST_BIG 0
#endif
#define FONT_BIG ((LOAD_UW(h + 66) & 0x04) != 0)

	if (!check_gemfnt_header(h, l))
	{
		swap_gemfnt_header(h, l);
		if (!check_gemfnt_header(h, l))
		{
			swap_gemfnt_header(h, l);
			fprintf(stderr, "%s: %s: invalid font header\n", program_name, filename);
			if (!info)
				exit(EXIT_FAILURE);
		} else
		{
			if (HOST_BIG == FONT_BIG)
			{
				if (!quiet)
					fprintf(stderr, "%s: warning: %s: wrong endian flag in header\n", program_name, filename);
				if (HOST_BIG)
					STORE_W(h + 66, LOAD_UW(h + 66) & ~0x0004);
				else
					STORE_W(h + 66, LOAD_UW(h + 66) | 0x0004);
			}
		}
	} else
	{
		if (HOST_BIG != FONT_BIG)
		{
			if (!quiet)
				fprintf(stderr, "%s: warning: %s: wrong endian flag in header\n", program_name, filename);
			if (HOST_BIG)
				STORE_W(h + 66, LOAD_UW(h + 66) | 0x0004);
			else
				STORE_W(h + 66, LOAD_UW(h + 66) & ~0x0004);
		}
	}
	
	firstc = LOAD_UW(h + 36);
	lastc = LOAD_UW(h + 38);

	flags = LOAD_UW(h + 66);
	form_width = LOAD_UW(h + 80);
	form_height = LOAD_UW(h + 82);
	
	top = LOAD_W(h + 40);
	max_cell_width = LOAD_W(h + 52);
	
	chomp(facename_buf, (const char *)h + 4, VDI_FONTNAMESIZE);
	strcpy(family_name, facename_buf);
	fix_name(family_name);
	if (strlen(family_name) > 11 && strcmp(family_name + strlen(family_name) - 11, "system font") == 0)
		strcpy(family_name, "atarisys");
	
	if (info)
	{
		fprintf(out, "Name: %s\n", facename_buf);
		fprintf(out, "Id: %d\n", LOAD_UW(h + 0));
		fprintf(out, "Size: %dpt\n", LOAD_UW(h + 2));
		fprintf(out, "First ade: %d\n", firstc);
		fprintf(out, "Last ade: %d\n", lastc);
		fprintf(out, "Top: %d\n", top);
		fprintf(out, "Ascent: %d\n", LOAD_W(h + 42));
		fprintf(out, "Half: %d\n", LOAD_W(h + 44));
		fprintf(out, "Descent: %d\n", LOAD_W(h + 46));
		fprintf(out, "Bottom: %d\n", LOAD_W(h + 48));
		fprintf(out, "Max charwidth: %d\n", LOAD_UW(h + 50));
		fprintf(out, "Max cellwidth: %d\n", max_cell_width);
		fprintf(out, "Left offset: %d\n", LOAD_W(h + 54));
		fprintf(out, "Right offset: %d\n", LOAD_W(h + 56));
		fprintf(out, "Thicken: %d\n", LOAD_UW(h + 58));
		fprintf(out, "Underline size: %d\n", LOAD_UW(h + 60));
		fprintf(out, "Lighten: $%x\n", LOAD_UW(h + 62));
		fprintf(out, "Skew: $%x\n", LOAD_UW(h + 64));
		fprintf(out, "Flags: $%x (%s%s%s-endian %s%s)\n", flags,
			flags & FONTF_SYSTEM ? "system " : "",
			flags & FONTF_HORTABLE ? "offsets " : "",
			flags & FONTF_BIGENDIAN ? "big" : "little",
			flags & FONTF_MONOSPACED ? "monospaced" : "proportional",
			flags & FONTF_EXTENDED ? " extended" : "");
		fprintf(out, "Horizontal table: %u\n", LOAD_UL(h + 68));
		fprintf(out, "Offset table: %u\n", LOAD_UL(h + 72));
		fprintf(out, "Data: %u\n", LOAD_UL(h + 76));
		fprintf(out, "Form width: %d\n", form_width);
		fprintf(out, "Form height: %d\n", form_height);
		return;
	}
	
	hor_table = h + LOAD_UL(h + 68);
	off_table = h + LOAD_UL(h + 72);
	dat_table = h + LOAD_UL(h + 76);
	numoffs = lastc - firstc + 1;

	if (HOST_BIG != FONT_BIG)
	{
		UB *u;
		
		for (u = off_table; u <= off_table + numoffs * 2; u += 2)
		{
			SWAP_W(u);
		}
		if ((flags & FONTF_HORTABLE) && hor_table != h && hor_table != off_table && (off_table - hor_table) >= (numoffs * 2))
		{
			for (u = hor_table; u < hor_table + numoffs * 2; u += 2)
			{
				SWAP_W(u);
			}
		}
	}
	
	mono = TRUE;
	{
		int o, w, firstw;
		
		o = LOAD_UW(off_table + 2 * 0);
		firstw = LOAD_UW(off_table + 2 * 0 + 2) - o;
		for (i = 0; i < numoffs; i++)
		{
			o = LOAD_UW(off_table + 2 * i);
			w = LOAD_UW(off_table + 2 * i + 2) - o;
			if (w != 0 && w != firstw)
			{
				mono = FALSE;
				if (flags & FONTF_MONOSPACED)
				{
					if (!quiet)
						fprintf(stderr, "%s: warning: %s: font says it is monospaced, but isn't\n", program_name, filename);
					flags &= ~FONTF_MONOSPACED;
					STORE_UW(h + 66, flags);
				}
				break;
			}
		}
		if (mono && !(flags & FONTF_MONOSPACED))
		{
			if (!quiet)
				fprintf(stderr, "%s: warning: %s: font does not say it is monospaced, but is\n", program_name, filename);
			flags |= FONTF_MONOSPACED;
			STORE_UW(h + 66, flags);
		}
	}
	
	if (verbose)
		fprintf(stderr, "Writing font %s (chars %d..%d)\n", facename_buf, firstc, lastc);
	fprintf(out, "STARTFONT 2.1\n");
	fprintf(out, "FONT %s\n", font_name(facename_buf, LOAD_UW(h + 2), max_cell_width, form_height, flags, LOAD_UW(h + 0)));
	fprintf(out, "SIZE %d 100 100\n", LOAD_UW(h + 2));
	fprintf(out, "FONTBOUNDINGBOX %d %d 0 0\n", max_cell_width, form_height);
	fprintf(out, "PIXEL_SIZE %d\n", form_height); /* needed by bdf2ttf */
	fprintf(out, "STARTPROPERTIES 12\n");
	fprintf(out, "FACE_NAME \"%s\"\n", facename_buf);
	fprintf(out, "FAMILY_NAME \"%s\"\n", family_name);
	fprintf(out, "FONT_ASCENT %d\n", top);
	fprintf(out, "FONT_DESCENT %d\n", form_height - top);
	fprintf(out, "SPACING \"%c\"\n", mono ? 'M' : 'P');
	if (translate)
	{
		fprintf(out, "CHARSET_REGISTRY \"ISO8859-1\"\n");
		fprintf(out, "CHARSET_ENCODING \"1\"\n");
	} else
	{
		fprintf(out, "CHARSET_REGISTRY \"ATARIST\"\n");
		fprintf(out, "CHARSET_ENCODING \"1\"\n");
	}
	fprintf(out, "X_HEIGHT %d\n", LOAD_W(h + 44));
	fprintf(out, "CAP_HEIGHT %d\n", LOAD_W(h + 42));
	default_char = '?';
	if (default_char < firstc || default_char > lastc)
		default_char = firstc;
	fprintf(out, "DEFAULT_CHAR %d\n", default_char);
	fprintf(out, "WEIGHT_NAME \"%s\"\n", WEIGHT);
#if 0
	/* if we put the foundry here, pcf files will report "FOUNDRY FAMILY" as family name :( */
	fprintf(out, "FOUNDRY \"%s\"\n", foundry);
#endif
	fprintf(out, "SLANT \"%s\"\n", SLANT);
	fprintf(out, "ENDPROPERTIES\n");
	fprintf(out, "CHARS %d\n", numoffs);
	for (i = 0; i < numoffs; i++)
	{
		int j;
		int o = LOAD_UW(off_table + 2 * i);
		int w = LOAD_UW(off_table + 2 * i + 2) - o;

		fprintf(out, "STARTCHAR dec%d\n", i + firstc);
		fprintf(out, "ENCODING %d\n", i + firstc < 256 ? tr[i + firstc] : i + firstc);
		fprintf(out, "SWIDTH %d 0\n", 100 * w);
		fprintf(out, "DWIDTH %d 0\n", w);
		fprintf(out, "BBX %d %d %d %d\n", w, form_height, 0, top - form_height);
		fprintf(out, "BITMAP\n");
		if (!nocomment)
		{
			for (j = 0; j < form_height; j++)
			{
				print_comment(dat_table + form_width * j, o, w, out);
			}
		}
		for (j = 0; j < form_height; j++)
		{
			print_line(dat_table + form_width * j, o, w, out);
		}
		fprintf(out, "ENDCHAR\n");
	}
	fprintf(out, "ENDFONT\n");
}



#include "../include/stcharmap.h"

static struct option const long_options[] = {
	{ "translate", no_argument, NULL, 't' },
	{ "info", no_argument, NULL, 'i' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "quiet", no_argument, NULL, 'q' },
	{ "nocomment", no_argument, NULL, 'c' },
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ NULL, no_argument, NULL, 0 }
};


static void usage(FILE *fp)
{
	fprintf(fp, "%s, convert Atari GEM font files to bdf format\n", program_name);
	fprintf(fp, "usage: %s [<options>] <file...>\n", program_name);
	fprintf(fp, "options:\n");
	fprintf(fp, "  -i, --info       only print font header info\n");
	fprintf(fp, "  -t, --translate  write bdf file with iso-latin-1 encoding\n");
	fprintf(fp, "  -v, --verbose    be verbose\n");
	fprintf(fp, "  -q, --quiet      be quiet\n");
	fprintf(fp, "  -c, --nocomment  do not write comments\n");
	fprintf(fp, "      --help       print this help and exit\n");
}


static void print_version(void)
{
}


int main(int argc, char **argv)
{
	FILE *in, *out;
	int l;
	int i;
	const char *filename = NULL;
	int c;
	
	while ((c = getopt_long(argc, argv, "tivqchV", long_options, NULL)) != EOF)
	{
		switch (c)
		{
		case 't':
			translate = TRUE;
			break;
		case 'i':
			info = TRUE;
			break;
		case 'v':
			verbose = TRUE;
			break;
		case 'q':
			quiet = TRUE;
			break;
		case 'c':
			nocomment = TRUE;
			break;
		case 'h':
			usage(stdout);
			exit(EXIT_SUCCESS);
			break;
		case 'V':
			print_version();
			exit(EXIT_SUCCESS);
			break;
		default:
			exit(EXIT_FAILURE);
			break;
		}
	}

	switch (argc - optind)
	{
	case 1:
		break;
	case 0:
		fprintf(stderr, "%s: no files specified\n", program_name);
		exit(EXIT_FAILURE);
		break;
	default:
		if (!info)
		{
			fprintf(stderr, "%s: too many files specified\n", program_name);
			exit(EXIT_FAILURE);
		}
		break;
	}

	if (translate)
	{
		for (i = 0; i < (int) NUMBER_OF_PAIRS; i++)
		{
			tr[known_pairs[i].st] = known_pairs[i].uni;
			invtr[known_pairs[i].uni] = known_pairs[i].st;
		}
	}
	for (i = 0; i < 256; i++)
	{
		if (tr[i] == 0 && invtr[i] == 0)
			tr[i] = i;
	}
	
	while (optind < argc)
	{
		filename = argv[optind];
		in = fopen(filename, "rb");
		if (in == NULL)
		{
			fprintf(stderr, "%s: ", program_name);
			perror(filename);
			exit(EXIT_FAILURE);
		}
		out = stdout;
		fseek(in, 0, SEEK_END);
		l = ftell(in);
		fseek(in, 0, SEEK_SET);
		m = (UB *)malloc(l);
		l = fread(m, 1, l, in);
	
		fnttobdf(m, l, out, filename);
		fclose(in);
		free(m);
		optind++;
	}
		
	return EXIT_SUCCESS;
}
