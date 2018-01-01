/***************************************************************************/
/*                                                                         */
/*  gemfnt.c                                                               */
/*                                                                         */
/*    FreeType font driver for GEM bitmap fonts                            */
/*                                                                         */
/*  Copyright 2013 by Thorsten Otto                                        */
/*                                                                         */
/*  Based on winfnt.c,                                                     */
/*  Copyright 1996-2004, 2006-2013 by                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*  Copyright 2003 Huw D M Davies for Codeweavers                          */
/*  Copyright 2007 Dmitry Timoshkov for Codeweavers                        */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include <ft2bld.h>
#include <ft2build.h>
#include <freetype/ftgemfnt.h>
#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/ftobjs.h>
#include <freetype/ttnameid.h>
#include <freetype/config/ftstdlib.h>

#include "gemfnt.h"
#include <freetype/ftmoderr.h>

ANONYMOUS_STRUCT_DUMMY(FT_RasterRec_)
ANONYMOUS_STRUCT_DUMMY(FT_Size_InternalRec_)
ANONYMOUS_STRUCT_DUMMY(FT_IncrementalRec_)

#undef __FTERRORS_H__

#undef  FT_ERR_PREFIX
#define FT_ERR_PREFIX  FNT_Err_
#define FT_ERR_BASE    FT_Mod_Err_Gemfonts

#include <freetype/fterrors.h>
#include <freetype/internal/services/svgemfnt.h>
#include <freetype/internal/services/svfntfmt.h>


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gemfnt


static const FT_Frame_Field gemfnt_header_fields[] = {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  FT_GemFNT_HeaderRec

	FT_FRAME_START(88),
	FT_FRAME_USHORT(font_id),
	FT_FRAME_USHORT(nominal_point_size),
	FT_FRAME_BYTES(face_name, 32),
	FT_FRAME_USHORT(first_ade),
	FT_FRAME_USHORT(last_ade),
	FT_FRAME_USHORT(top),
	FT_FRAME_USHORT(ascent),
	FT_FRAME_USHORT(half),
	FT_FRAME_USHORT(descent),
	FT_FRAME_USHORT(bottom),
	FT_FRAME_USHORT(max_char_width),
	FT_FRAME_USHORT(max_cell_width),
	FT_FRAME_SHORT(left_offset),
	FT_FRAME_SHORT(right_offset),
	FT_FRAME_USHORT(thicken),
	FT_FRAME_USHORT(underline_size),
	FT_FRAME_USHORT(lighten),
	FT_FRAME_USHORT(skew),
	FT_FRAME_USHORT(flags),
	FT_FRAME_ULONG(horizontal_table_offset),
	FT_FRAME_ULONG(offset_table_offset),
	FT_FRAME_ULONG(data_table_offset),
	FT_FRAME_USHORT(form_width),
	FT_FRAME_USHORT(form_height),
	FT_FRAME_ULONG(next_font),
	FT_FRAME_END
};


static void fnt_font_done(FNT_Face face)
{
	FT_Memory memory = FT_FACE(face)->memory;
	FT_Stream stream = FT_FACE(face)->stream;
	FNT_Font font = face->font;

	if (!font)
		return;

	if (font->fnt_data)
		FT_FRAME_RELEASE(font->fnt_data);
	FT_FREE(font->family_name);
	FT_FREE(font->horizontal_offsets);
	FT_FREE(font->offsets);

	FT_FREE(font);
	face->font = 0;
}


/*
 * Theoretically, there is a flag in the header telling wether the
 * font is in little- or big-endian format.
 * Unfortunately, this header field is a 2-byte value, thus we
 * would have to now the format before even accessing this field.
 * Moreover, there are apparantly several fonts that have the
 * big-endian flag set but are stored in little-endian format.
 * Thus we do some checks instead for validity of the fields,
 * which is a bit tricky since the font format does not even
 * contain a magic field.
 * The code below assumes that
 * - there is no valid font file with a point size of less
 *   than 3 or greater than 768
 * - there is no valid font file with a starting codepoint
 *   of 0x2000 or higher
 * - there is no valid font file with a ending codepoint
 *   of 0xff00 or higher
 */
static int check_gemfnt_header(FT_GemFNT_Header header, size_t l)
{
	FT_UShort firstc, lastc, points;
	FT_UShort form_width, form_height;
	FT_ULong dat_offset;
	FT_ULong off_table;

	/*
	 * 84, because the last field is a pointer to the next font,
	 * which is only valid when already loaded in memory,
	 * and thus is not neccessary contained in the font file
	 */
	if (l < 84)
		return FALSE;
	firstc = header->first_ade;
	lastc = header->last_ade;
	points = header->nominal_point_size;
	if (lastc == 256)
	{
		/*
		 * kludge. some font designers seem to have
		 * misunderstood the meaning of the "last character" field
		 */
		lastc = 255;
	}
	if (firstc >= 0x2000 || lastc >= 0xff00 || firstc > lastc)
		return FALSE;
	if (points >= 0x300)
		return FALSE;
	if (header->horizontal_table_offset >= l)
		return FALSE;
	off_table = header->offset_table_offset;
	if (off_table < 84 || (off_table + (lastc - firstc + 1) * 2) > l)
		return FALSE;
	dat_offset = header->data_table_offset;
	if (dat_offset < 84 || dat_offset >= l)
		return FALSE;
	if (header->max_cell_width == 0)
		return FALSE;
	form_width = header->form_width;
	form_height = header->form_height;
	if ((dat_offset + form_width * form_height) > l)
		return FALSE;
	header->last_ade = lastc;
	return TRUE;
}


static int host_big(void)
{
	static unsigned char const bytes[2] = { 0x12, 0x34 };
	const FT_UInt16 *p = ((const FT_UInt16 *)bytes);
	return *p == 0x1234;
}


static FT_UInt16 swap_short(FT_UInt16 val)
{
	return ((val & 0xff) << 8) |
	       ((val >> 8) & 0xff);
}


static FT_UInt32 swap_long(FT_UInt32 val)
{
	return ((val & 0xff) << 24) |
	       ((val & 0xff00) << 8) |
	       ((val >> 8) & 0xff00) |
	       ((val >> 24) & 0xff);
}


static void swap_gemfnt_header(FT_GemFNT_Header header, size_t l)
{
	if (l < 84)
		return;

#define SWAP_W(field) header->field = swap_short(header->field)
#define SWAP_L(field) header->field = swap_long(header->field)

	SWAP_W(font_id);
	SWAP_W(nominal_point_size);
	/* skip name */
	SWAP_W(first_ade);
	SWAP_W(last_ade);
	SWAP_W(top);
	SWAP_W(ascent);
	SWAP_W(half);
	SWAP_W(descent);
	SWAP_W(bottom);
	SWAP_W(max_char_width);
	SWAP_W(max_cell_width);
	SWAP_W(left_offset);
	SWAP_W(right_offset);
	SWAP_W(thicken);
	SWAP_W(underline_size);
	SWAP_W(lighten);
	SWAP_W(skew);
	SWAP_W(flags);
	SWAP_L(horizontal_table_offset);
	SWAP_L(offset_table_offset);
	SWAP_L(data_table_offset);
	SWAP_W(form_width);
	SWAP_W(form_height);
	
#undef SWAP_W
#undef SWAP_L
}



#define FT_GEMFNT_FLAG_SYSTEM     0x01
#define FT_GEMFNT_FLAG_HORTABLE   0x02
#define FT_GEMFNT_FLAG_BIGENDIAN  0x04
#define FT_GEMFNT_FLAG_MONOSPACED 0x08


static FT_Error fnt_font_load(FNT_Face face, FNT_Font font, FT_Stream stream)
{
	FT_Error error;
	FT_GemFNT_Header header = &font->header;
	int hortable_bytes;
	int numoffs;
	FT_Memory memory = FT_FACE_MEMORY(face);
	FT_ULong datasize;
	
	/* first of all, read the FNT header */
	if (FT_STREAM_SEEK(font->offset) || FT_STREAM_READ_FIELDS(gemfnt_header_fields, header))
		return error;

	/*
	 * check header
	 * remember that the field definitions above describe a header in big-endian format
	 */
	if (!check_gemfnt_header(header, font->fnt_size))
	{
		swap_gemfnt_header(header, font->fnt_size);
		if (!check_gemfnt_header(header, font->fnt_size))
		{
			swap_gemfnt_header(header, font->fnt_size);
			FT_TRACE2(("  not a GEM FNT file\n"));
			return FT_THROW(Unknown_File_Format);
		}
		if (header->flags & FT_GEMFNT_FLAG_BIGENDIAN)
		{
			FT_TRACE2(("warning: %s: wrong endian flag in header\n", stream->pathname.pointer));
			if (host_big())
			{
				/*
				 * host big-endian, font claims to be little-endian,
				 * but check succeded only after swapping:
				 * font apparently is big-endian, set flag
				 */
				header->flags |= FT_GEMFNT_FLAG_BIGENDIAN;
			} else
			{
				/*
				 * host little-endian, font claims to be big-endian,
				 * but check succeded only after swapping:
				 * font apparently is little-endian, clear flag
				 */
				 header->flags &= ~FT_GEMFNT_FLAG_BIGENDIAN;
			}
		}
	} else
	{
		if (!(header->flags & FT_GEMFNT_FLAG_BIGENDIAN))
		{
			FT_TRACE2(("warning: %s: wrong endian flag in header\n", stream->pathname.pointer));
			if (host_big())
			{
				/*
				 * host big-endian, font claims to be little-endian,
				 * but check succeded without swapping:
				 * font apparently is big-endian, set flag
				 */
				header->flags |= FT_GEMFNT_FLAG_BIGENDIAN;
			} else
			{
				/*
				 * host little-endian, font claims to be big-endian,
				 * but check succeded without swapping:
				 * font apparently is little-endian, clear flag
				 */
				header->flags &= ~FT_GEMFNT_FLAG_BIGENDIAN;
			}
		}
	}

	numoffs = header->last_ade - header->first_ade + 1;

	/*
	 * Load the horizontal offset table.
	 * There are very few fonts that use it,
	 * and of those that do, some apparently store a table containing shorts,
	 * although the documentation says it is byte size only.
	 * Several font files also have a table that is completely empty.
	 */
	hortable_bytes = 0;
	if ((header->flags & FT_GEMFNT_FLAG_HORTABLE) &&
		header->horizontal_table_offset != 0 &&
		header->horizontal_table_offset != header->offset_table_offset)
	{
		if ((int)(header->offset_table_offset - header->horizontal_table_offset) >= (numoffs * 2))
			hortable_bytes = 2;
		else
			hortable_bytes = 1;
	}
	
	if (header->flags & FT_GEMFNT_FLAG_HORTABLE)
	{
		int empty = TRUE;

		if (hortable_bytes == 0)
		{
			FT_TRACE2(("warning: %s: offset flag but no table given\n", stream->pathname.pointer));
			header->flags &= ~FT_GEMFNT_FLAG_HORTABLE;
		} else
		{
			FT_ULong tmp_size = numoffs * hortable_bytes;
			FT_ULong hortable_size = numoffs * sizeof(*font->horizontal_offsets);
			FT_Byte *tmp_table;
			const FT_Byte *u;
			int i;
			
			if (FT_STREAM_SEEK(font->offset + header->horizontal_table_offset) ||
				FT_FRAME_EXTRACT(tmp_size, tmp_table))
				return error;
			if (FT_ALLOC(font->horizontal_offsets, hortable_size))
			{
				FT_FRAME_RELEASE(tmp_table);
				return error;
			}
			
			if (hortable_bytes == 2)
			{
				for (u = tmp_table, i = 0; i < numoffs; u += 2, i++)
				{
					if (!(header->flags & FT_GEMFNT_FLAG_BIGENDIAN))
						font->horizontal_offsets[i] = FT_PEEK_SHORT_LE(u);
					else
						font->horizontal_offsets[i] = FT_PEEK_SHORT(u);
						
					if (font->horizontal_offsets[i] != 0)
						empty = FALSE;
				}
			}
			if (hortable_bytes == 1)
			{
				for (u = tmp_table, i = 0; i < numoffs; u++, i++)
				{
					font->horizontal_offsets[i] = FT_INT8_(u, 0);
					if (font->horizontal_offsets[i] != 0)
						empty = FALSE;
				}
			}
			if (hortable_bytes != 0 && empty)
			{
				FT_TRACE2(("warning: %s: empty horizontal offset table\n", stream->pathname.pointer));
				FT_FREE(font->horizontal_offsets);
				header->flags &= ~FT_GEMFNT_FLAG_HORTABLE;
			}
			FT_FRAME_RELEASE(tmp_table);
		}
	} else
	{
		if (header->horizontal_table_offset != 0 &&
			header->horizontal_table_offset != header->offset_table_offset)
		{
			FT_TRACE2(("warning: %s: offset table but flag not set; ignored\n", stream->pathname.pointer));
		}
	}

	/*
	 * Load the offsets table.
	 */
	{
		FT_Byte *tmp_table;
		const FT_Byte *u;
		int i;
		FT_ULong tmp_size = (numoffs + 1) * 2;
		FT_ULong table_size = (numoffs + 1) * sizeof(*(font->offsets));
		
		if (FT_STREAM_SEEK(font->offset + header->offset_table_offset) ||
			FT_FRAME_EXTRACT(tmp_size, tmp_table))
			return error;
		if (FT_ALLOC(font->offsets, table_size))
		{
			FT_FRAME_RELEASE(tmp_table);
			return error;
		}
		if (!(header->flags & FT_GEMFNT_FLAG_BIGENDIAN))
		{
			for (i = 0, u = tmp_table; i <= numoffs; i++, u += 2)
			{
				font->offsets[i] = FT_PEEK_SHORT_LE(u);
			}
		} else
		{
			for (i = 0, u = tmp_table; i <= numoffs; i++, u += 2)
			{
				font->offsets[i] = FT_PEEK_SHORT(u);
			}
		}
		FT_FRAME_RELEASE(tmp_table);
	}

	/* extract the data */
	datasize = font->header.form_width * font->header.form_height;
	if (FT_STREAM_SEEK(font->offset + font->header.data_table_offset) || FT_FRAME_EXTRACT(datasize, font->fnt_data))
		return error;
	
	return FT_Err_Ok;
}


typedef struct FNT_CMapRec_
{
	FT_CMapRec cmap;
	FT_UInt32 first;
	FT_UInt32 count;

} FNT_CMapRec, *FNT_CMap;


static FT_Error fnt_cmap_init(FT_CMap ft_cmap, FT_Pointer data)
{
	FNT_CMap cmap = (FNT_CMap) ft_cmap;
	FNT_Face face = (FNT_Face) FT_CMAP_FACE(cmap);
	FNT_Font font = face->font;

	FT_UNUSED(data);
	
	cmap->first = (FT_UInt32) font->header.first_ade;
	cmap->count = (FT_UInt32) (font->header.last_ade - font->header.first_ade + 1);

	return FT_Err_Ok;
}


static FT_UInt fnt_cmap_char_index(FT_CMap ft_cmap, FT_UInt32 char_code)
{
	FNT_CMap cmap = (FNT_CMap) ft_cmap;
	FT_UInt gindex = 0;

	char_code -= cmap->first;
	if (char_code < cmap->count)
		/* we artificially increase the glyph index; */
		/* FNT_Load_Glyph reverts to the right one   */
		gindex = (FT_UInt)(char_code + 1);
	return gindex;
}


static FT_UInt fnt_cmap_char_next(FT_CMap ft_cmap, FT_UInt32 *pchar_code)
{
	FNT_CMap cmap = (FNT_CMap) ft_cmap;
	FT_UInt gindex = 0;
	FT_UInt32 result = 0;
	FT_UInt32 char_code = *pchar_code + 1;

	if (char_code <= cmap->first)
	{
		result = cmap->first;
		gindex = 1;
	} else
	{
		char_code -= cmap->first;
		if (char_code < cmap->count)
		{
			result = cmap->first + char_code;
			gindex = (FT_UInt)(char_code + 1);
		}
	}

	*pchar_code = result;
	return gindex;
}


static const FT_CMap_ClassRec fnt_cmap_class_rec = {
	sizeof(FNT_CMapRec),

	fnt_cmap_init,
	NULL,					/* FT_CMap_DoneFunc */
	fnt_cmap_char_index,
	fnt_cmap_char_next,
	NULL,					/* FT_CMap_CharVarIndexFunc */
	NULL,					/* FT_CMap_CharVarIsDefaultFunc */
	NULL,					/* FT_CMap_VariantListFunc */
	NULL,					/* FT_CMap_CharVariantListFunc */
	NULL					/* FT_CMap_VariantCharListFunc */
};

static FT_CMap_Class const fnt_cmap_class = &fnt_cmap_class_rec;


static void FNT_Face_Done(FT_Face fntface)	/* FNT_Face */
{
	FNT_Face face = (FNT_Face) fntface;
	FT_Memory memory;

	if (!face)
		return;

	memory = FT_FACE_MEMORY(face);

	fnt_font_done(face);

	FT_FREE(fntface->available_sizes);
	fntface->num_fixed_sizes = 0;
}


static int ft_strcaseeq(const char *s1, const char *s2)
{
	while (*s1 && *s2)
	{
		char c1 = ft_islower(*s1) ? *s1 - 'a' + 'A' : *s1;
		char c2 = ft_islower(*s2) ? *s2 - 'a' + 'A' : *s2;
		if (c1 != c2)
			return FALSE;
		s1++;
		s2++;
	}
	return *s1 == *s2;
}


static FT_Long fix_name(FNT_Font font)
{
	char *name = font->family_name;
	size_t len = ft_strlen(name);
	FT_Long style_flags = 0;
	
	while (len > 0)
	{
		while (len > 0 && name[len - 1] == ' ')
			name[--len] = '\0';
		if (len > 0 && ft_isdigit(name[len - 1]))
		{
			while (len > 0 && ft_isdigit(name[len - 1]))
				name[--len] = '\0';
			if (len > 0 && name[len - 1] == 'x')
			{
				name[--len] = '\0';
				while (len > 0 && ft_isdigit(name[len - 1]))
					name[--len] = '\0';
			}
			continue;
		}
		if (len > 4 && ft_strcaseeq(name + len - 4, "bold"))
		{
			len -= 4;
			name[len] = '\0';
			style_flags |= FT_STYLE_FLAG_BOLD;
			continue;
		}
		if (len > 5 && ft_strcaseeq(name + len - 5, "punkt"))
		{
			len -= 5;
			name[len] = '\0';
			continue;
		}
		if (len > 9 && ft_strcaseeq(name + len - 9, "monospace"))
		{
			len -= 9;
			name[len] = '\0';
			continue;
		}
		if (len > 12 && ft_strcaseeq(name + len - 12, "proportional"))
		{
			len -= 12;
			name[len] = '\0';
			continue;
		}
		break;
	}
	while (len > 0 && name[0] == ' ')
	{
		ft_memmove(name, name + 1, len);
		len--;
	}
	return style_flags;
}


static FT_Error FNT_Face_Init(FT_Stream stream, FT_Face fntface,	/* FNT_Face */
							  FT_Int face_index, FT_Int num_params, FT_Parameter * params)
{
	FNT_Face face = (FNT_Face) fntface;
	FT_Error error;
	FT_Memory memory = FT_FACE_MEMORY(face);
	FNT_Font font;

	FT_UNUSED(num_params);
	FT_UNUSED(params);

	FT_TRACE2(("GEM FNT driver\n"));

	if (FT_NEW(face->font))
		goto Exit;

	fntface->num_faces = 1;

	font = face->font;
	font->offset = 0;
	font->fnt_size = stream->size - font->offset;
	font->default_char = '?';
	
	error = fnt_font_load(face, font, stream);

	if (!error)
	{
		if (face_index > 0)
			error = FT_THROW(Invalid_Argument);
		else if (face_index < 0)
			goto Exit;
	}

	if (error)
		goto Fail;

	/* we now need to fill the root FT_Face fields */
	/* with relevant information                   */
	{
		FT_Face root = FT_FACE(face);
		FNT_Font font = face->font;
		FT_PtrDist family_size;

		root->face_index = face_index;

		root->face_flags = FT_FACE_FLAG_FIXED_SIZES | FT_FACE_FLAG_HORIZONTAL;

		family_size = 32;
		/* Some broken fonts don't delimit the face name with a final */
		/* NULL byte -- the frame is erroneously one byte too small.  */
		/* We thus allocate one more byte, setting it explicitly to   */
		/* zero.                                                      */
		if (FT_ALLOC(font->family_name, family_size + 1))
			goto Fail;

		FT_MEM_COPY(font->family_name, font->header.face_name, family_size);

		font->family_name[family_size] = '\0';
		root->style_flags |= fix_name(font);
		
		/*
		 * hacks for small Atari ROM builtin fonts,
		 * which where designed for different resolutions
		 */
		if (ft_strcmp(font->family_name, "6x6 system font") == 0)
			font->header.nominal_point_size = 4;
		else if (ft_strcmp(font->family_name, "8x8 system font") == 0)
			font->header.nominal_point_size = 5;

		/*
		 * hacks for Atari ROM builtin fonts,
		 * which have different names depending on size.
		 */
		if (ft_strcmp(font->family_name, "6x6 system font") == 0 ||
			ft_strcmp(font->family_name, "8x8 system font") == 0 ||
			ft_strcmp(font->family_name, "8x16 system font") == 0 ||
			ft_strcmp(font->family_name, "16x32 system font") == 0)
			ft_strcpy(font->family_name, "AtariSYS");
		
		if (FT_REALLOC(font->family_name, family_size + 1, ft_strlen(font->family_name) + 1))
			goto Fail;

		if (font->header.flags & FT_GEMFNT_FLAG_MONOSPACED)
			root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

		/* set up the `fixed_sizes' array */
		if (FT_NEW_ARRAY(root->available_sizes, 1))
			goto Fail;

		root->num_fixed_sizes = 1;

		{
			FT_Bitmap_Size *bsize = root->available_sizes;
			FT_UShort x_res, y_res;

			bsize->width = font->header.max_char_width;
			bsize->height = (FT_Short) (font->header.form_height);
			bsize->size = font->header.nominal_point_size << 6;

			x_res = y_res = (font->header.top * 72) / font->header.nominal_point_size;
			x_res = y_res = 96;
			
			bsize->y_ppem = FT_MulDiv(bsize->size, y_res, 72);
			bsize->y_ppem = font->header.nominal_point_size << 6;
			bsize->y_ppem = FT_PIX_ROUND(bsize->y_ppem);

			/*
			 * this reads:
			 *
			 * the nominal height is larger than the bbox's height
			 *
			 * => nominal_point_size contains incorrect value;
			 *    use pixel_height as the nominal height
			 */
			if (bsize->y_ppem > (font->header.form_height << 6))
			{
				FT_TRACE2(("use pixel_height as the nominal height\n"));

				bsize->y_ppem = font->header.form_height << 6;
				bsize->size = FT_MulDiv(bsize->y_ppem, 72, y_res);
			}

			bsize->x_ppem = FT_MulDiv(bsize->size, x_res, 72);
			bsize->x_ppem = FT_PIX_ROUND(bsize->x_ppem);
		}

		{
			FT_CharMapRec charmap;

			charmap.encoding = FT_ENCODING_NONE;
			/* initial platform/encoding should indicate unset status? */
			charmap.platform_id = TT_PLATFORM_CUSTOM;
			charmap.encoding_id = TT_APPLE_ID_DEFAULT;
			charmap.face = root;

			error = FT_CMap_New(fnt_cmap_class, NULL, &charmap, NULL);
			if (error)
				goto Fail;

			/* Select default charmap */
			if (root->num_charmaps)
				root->charmap = root->charmaps[0];
		}

		/* set up remaining flags */

		if (font->header.last_ade < font->header.first_ade)
		{
			FT_TRACE2(("invalid number of glyphs\n"));
			error = FT_THROW(Invalid_File_Format);
			goto Fail;
		}

		/* reserve one slot for the .notdef glyph at index 0 */
		root->num_glyphs = font->header.last_ade - font->header.first_ade + 1 + 1;

		root->family_name = font->family_name;
		root->style_name = (char *) "Regular";

		if (root->style_flags & FT_STYLE_FLAG_BOLD)
		{
			if (root->style_flags & FT_STYLE_FLAG_ITALIC)
				root->style_name = (char *) "Bold Italic";
			else
				root->style_name = (char *) "Bold";
		} else if (root->style_flags & FT_STYLE_FLAG_ITALIC)
		{
			root->style_name = (char *) "Italic";
		}
	}
	goto Exit;

  Fail:
	FNT_Face_Done(fntface);

  Exit:
	return error;
}


static FT_Error FNT_Size_Select(FT_Size size, FT_ULong strike_index)
{
	FNT_Face face = (FNT_Face) size->face;
	FT_GemFNT_Header header = &face->font->header;

	FT_UNUSED(strike_index);

	FT_Select_Metrics(size->face, 0);

	size->metrics.ascender = header->top * 64;
	size->metrics.descender = -(header->form_height - header->top) * 64;
	size->metrics.max_advance = header->max_cell_width * 64;

	return FT_Err_Ok;
}


static FT_Error FNT_Size_Request(FT_Size size, FT_Size_Request req)
{
	FNT_Face face = (FNT_Face) size->face;
	FT_GemFNT_Header header = &face->font->header;
	FT_Bitmap_Size *bsize = size->face->available_sizes;
	FT_Error error = FT_Err_Ok;
	FT_Long height;

	height = FT_REQUEST_HEIGHT(req);
	height = (height + 32) >> 6;

	switch (req->type)
	{
	case FT_SIZE_REQUEST_TYPE_NOMINAL:
		if (height != ((bsize->y_ppem + 32) >> 6))
			error = FT_THROW(Invalid_Pixel_Size);
		break;

	case FT_SIZE_REQUEST_TYPE_REAL_DIM:
		if (height != header->form_height)
			error = FT_THROW(Invalid_Pixel_Size);
		break;

	default:
		error = FT_THROW(Unimplemented_Feature);
		break;
	}

	if (error != FT_Err_Ok)
		return error;
	return FNT_Size_Select(size, 0);
}


static FT_Error FNT_Load_Glyph(FT_GlyphSlot slot, FT_Size size, FT_UInt glyph_index, FT_Int32 load_flags)
{
	FNT_Face face = (FNT_Face) FT_SIZE_FACE(size);
	FNT_Font font;
	FT_Error error = FT_Err_Ok;
	FT_Bitmap *bitmap = &slot->bitmap;
	FT_UInt16 offset, width;
	
	FT_UNUSED(load_flags);

	if (!face)
	{
		error = FT_THROW(Invalid_Argument);
		goto Exit;
	}

	font = face->font;

	if (!font || glyph_index >= (FT_UInt) (FT_FACE(face)->num_glyphs))
	{
		error = FT_THROW(Invalid_Argument);
		goto Exit;
	}

	if (glyph_index > 0)
		glyph_index--;					/* revert to real index */
	else
		glyph_index = font->default_char;				/* the .notdef glyph */

	/* get glyph offset & width */
	offset = font->offsets[glyph_index];
	width = font->offsets[glyph_index + 1] - offset;
	
	if ((offset + width) > (font->header.form_width << 3))
	{
		FT_TRACE2(("invalid bitmap width\n"));
		error = FT_THROW(Invalid_File_Format);
		goto Exit;
	}

	bitmap->width = width;
	bitmap->rows = font->header.form_height;
	
	/* allocate and build bitmap */
	{
		FT_Memory memory = FT_FACE_MEMORY(slot->face);
		FT_Int pitch = (bitmap->width + 7) >> 3;
		FT_UInt16 x, y;
		
		bitmap->pitch = pitch;
		bitmap->pixel_mode = FT_PIXEL_MODE_MONO;

		/* note: since glyphs may start in the middle of a byte */
		/* we can't use ft_glyphslot_set_bitmap                 */
		if (FT_ALLOC_MULT(bitmap->buffer, pitch, bitmap->rows))
			goto Exit;
		ft_memset(bitmap->buffer, 0, pitch * bitmap->rows);
		
		for (y = 0; y < bitmap->rows; y++)
		{
			FT_Byte *dat;
			FT_Byte *out;
			int b, j;
			FT_Byte inmask, outmask;
			
			b = offset & 7;
			dat = font->fnt_data + font->header.form_width * y + (offset >> 3);
			out = bitmap->buffer + y * pitch;
			inmask = 0x80 >> b;
			outmask = 0x80;
			
			for (x = 0, j = 0; x < width; x++)
			{
				if ((*dat) & inmask)
					*out |= outmask;
				inmask >>= 1;
				b++;
				if (b == 8)
				{
					dat++;
					inmask = 0x80;
					b = 0;
				}
				outmask >>= 1;
				j++;
				if (j == 8)
				{
					out++;
					j = 0;
					outmask = 0x80;
				}
			}
		}
	}

	slot->internal->flags = FT_GLYPH_OWN_BITMAP;
	slot->bitmap_left = 0;
	slot->bitmap_top = font->header.top;
	slot->format = FT_GLYPH_FORMAT_BITMAP;

	/* now set up metrics */
	slot->metrics.width = bitmap->width << 6;
	slot->metrics.height = bitmap->rows << 6;
	slot->metrics.horiAdvance = bitmap->width << 6;
	slot->metrics.horiBearingX = 0;
	slot->metrics.horiBearingY = slot->bitmap_top << 6;

	ft_synthesize_vertical_metrics(&slot->metrics, bitmap->rows << 6);

  Exit:
	return error;
}


static FT_Error gemfnt_get_header(FT_Face face, FT_GemFNT_HeaderRec * aheader)
{
	FNT_Font font = ((FNT_Face) face)->font;

	*aheader = font->header;

	return 0;
}


static const FT_Service_GemFntRec gemfnt_service_rec = {
	gemfnt_get_header
};


 /*
  *  SERVICE LIST
  *
  */

static const FT_ServiceDescRec gemfnt_services[] = {
	{ FT_SERVICE_ID_FONT_FORMAT, FT_FONT_FORMAT_GEMFNT },
	{ FT_SERVICE_ID_GEMFNT, &gemfnt_service_rec },
	{ NULL, NULL }
};


static FT_Module_Interface gemfnt_get_service(FT_Module module, const FT_String *service_id)
{
	FT_UNUSED(module);

	return ft_service_list_lookup(gemfnt_services, service_id);
}


FT_CALLBACK_TABLE_DEF const FT_Driver_ClassRec gemfnt_driver_class = {
	{
		FT_MODULE_FONT_DRIVER | FT_MODULE_DRIVER_NO_OUTLINES,
		sizeof(FT_DriverRec),			/* module_size */

		"gemfonts",
		0x10000L,						/* module_version */
		0x20000L,						/* module_requires */

		0,								/* module_interface */

		0,								/* FT_Module_Constructor module_init */
		0,								/* FT_Module_Destructor module_done */
		gemfnt_get_service
	},

	sizeof(FNT_FaceRec),
	sizeof(FT_SizeRec),
	sizeof(FT_GlyphSlotRec),

	FNT_Face_Init,
	FNT_Face_Done,
	0,									/* FT_Size_InitFunc */
	0,									/* FT_Size_DoneFunc */
	0,									/* FT_Slot_InitFunc */
	0,									/* FT_Slot_DoneFunc */

	FNT_Load_Glyph,

	0,									/* FT_Face_GetKerningFunc  */
	0,									/* FT_Face_AttachFunc      */
	0,									/* FT_Face_GetAdvancesFunc */

	FNT_Size_Request,
	FNT_Size_Select
};
