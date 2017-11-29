#ifndef FONTHDR_H
#define FONTHDR_H

#define VDI_FONTNAMESIZE 32

typedef struct FONT_HDR
{
/*  0 */       short font_id;
/*  2 */       short point;
/*  4 */       char name[VDI_FONTNAMESIZE];
/* 36 */       unsigned short first_ade;
/* 38 */       unsigned short last_ade;
/* 40 */       unsigned short top;
/* 42 */       unsigned short ascent;
/* 44 */       unsigned short half;
/* 46 */       unsigned short descent;
/* 48 */       unsigned short bottom;
/* 50 */       unsigned short max_char_width;
/* 52 */       unsigned short max_cell_width;
/* 54 */       unsigned short left_offset;          /* amount character slants left when skewed */
/* 56 */       unsigned short right_offset;         /* amount character slants right */
/* 58 */       unsigned short thicken;              /* number of pixels to smear when bolding */
/* 60 */       unsigned short ul_size;              /* height of the underline */
/* 62 */       unsigned short lighten;              /* mask for lightening  */
/* 64 */       unsigned short skew;                 /* mask for skewing */
/* 66 */       unsigned short flags;                /* see below */
/* 68 */       uint32_t hor_table;                  /* horizontal offsets */
/* 72 */       uint32_t off_table;                  /* character offsets  */
/* 76 */       uint32_t dat_table;                  /* character definitions (raster data) */
/* 80 */       unsigned short form_width;           /* width of raster in bytes */
/* 82 */       unsigned short form_height;          /* height of raster in lines */
/* 84 */       uint32_t next_font;                  /* pointer to next font */
/* 88 */       unsigned short next_seg;				/* new */
/* 90 */
} FONT_HDR;


/* #define SIZEOF_FONT_HDR 90 in memory only */
#define SIZEOF_FONT_HDR 88

/* definitions for flags */
#define FONTF_SYSTEM     0x0001            /* Default system font */
#define FONTF_HORTABLE   0x0002            /* Use horizontal offsets table */
#define FONTF_BIGENDIAN  0x0004            /* Font image is in byteswapped format */
#define FONTF_MONOSPACED 0x0008            /* Font is monospaced */
#define FONTF_EXTENDED   0x0020            /* Extended font header */
#define FONTF_FULLID     0x2000            /* Use 'full font ID' */

#endif /* FONTHDR_H */
