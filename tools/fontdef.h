#ifndef __FONTDEF_H__
#define __FONTDEF_H__

/*
 * fontdef.h - font-header definitions
 *
 */

/* fh_flags   */

#define F_DEFAULT    1					/* this is the default font (face and size) */
#define F_HORZ_OFF   2					/* there are left and right offset tables */
#define F_STDFORM    4					/* is the font in standard (bigendian) format */
#define F_MONOSPACE  8					/* is the font monospaced */
#define F_EXT_HDR    32					/* extended header */

/* flags that this tool supports */
#define F_SUPPORTED (F_STDFORM|F_MONOSPACE|F_DEFAULT|F_HORZ_OFF|F_EXT_HDR)


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
	uint8_t font_id[2];
	uint8_t point[2];
	char name[32];
	uint8_t first_ade[2];
	uint8_t last_ade[2];
	uint8_t top[2];
	uint8_t ascent[2];
	uint8_t half[2];
	uint8_t descent[2];
	uint8_t bottom[2];
	uint8_t max_char_width[2];
	uint8_t max_cell_width[2];
	uint8_t left_offset[2];				/* amount character slants left when skewed */
	uint8_t right_offset[2];			/* amount character slants right */
	uint8_t thicken[2];					/* number of pixels to smear */
	uint8_t ul_size[2];					/* size of the underline */
	uint8_t lighten[2];					/* mask to and with to lighten  */
	uint8_t skew[2];					/* mask for skewing */
	uint8_t flags[2];

	uint8_t hor_table[4];				/* offset of horizontal offsets */
	uint8_t off_table[4];				/* offset of character offsets  */
	uint8_t dat_table[4];				/* offset of character definitions */
	uint8_t form_width[2];
	uint8_t form_height[2];
	uint8_t next_font[4];
};


struct boundingbox {
    int width;
    int height;
	int x;
	int y;
};

struct glyph {
	int id;
	uint16_t idx;
	int width;
	struct boundingbox bbx;
};

struct font
{										/* describes a font in memory */
	int16_t font_id;
	int16_t point;
	char name[32];
	uint16_t first_ade;
	uint16_t last_ade;
	uint16_t top;
	uint16_t ascent;
	uint16_t half;
	uint16_t descent;
	uint16_t bottom;
	uint16_t max_char_width;
	uint16_t max_cell_width;
	uint16_t left_offset;				/* amount character slants left when skewed */
	uint16_t right_offset;				/* amount character slants right */
	uint16_t thicken;					/* number of pixels to smear */
	uint16_t ul_size;					/* size of the underline */
	uint16_t lighten;					/* mask to and with to lighten  */
	uint16_t skew;						/* mask for skewing */
	uint16_t flags;

	uint8_t *hor_table;					/* horizontal offsets, 2 bytes per character */
	unsigned long *off_table;			/* character offsets, 0xFFFF if no char present. */
	uint8_t *dat_table;					/* character definitions */
	unsigned long form_width;
	uint16_t form_height;
	
	/*
	 * for TTF output only:
	 */
	int flagAutoName;
	int flagBold;
	int flagItalic;
	struct glyph *glyphs;
	int num_glyphs;
	int emX;
	int emY;
	int emDescent;
	int emAscent;
};

#define F_NO_CHAR 0xFFFFu
#define F_NO_CHARL 0xFFFFFFFFul

#define MAX_GLYPH 0x10000

extern const char	*g_copyright;
extern const char	*g_fontname;
extern const char	*g_version;
extern const char	*g_trademark;

void ttf_output(struct font *font, FILE *fp);

#endif /* __FONTDEF_H__ */
