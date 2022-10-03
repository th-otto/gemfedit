#ifndef __TRUETYPE_TABLES_H__
#define __TRUETYPE_TABLES_H__ 1

#include <stdint.h>

typedef unsigned char tt_uint16[2];
typedef unsigned char tt_uint32[4];
typedef unsigned char tt_int16[2];
typedef unsigned char tt_int32[4];

typedef tt_uint16 tt_offset16;
typedef tt_uint32 tt_offset32;

/* TTAG_head */
typedef struct
{
	tt_uint32    table_version;
	tt_uint32    font_revision;
	
	tt_int32     checksum_adjust;
	tt_int32     magic_number;
	
	tt_uint16    flags;
	tt_uint16    units_per_em;
	
	tt_uint32    created[2];
	tt_uint32    modified[2];
	
	tt_int16     xmin;
	tt_int16     ymin;
	tt_int16     xmax;
	tt_int16     ymax;
	
	tt_uint16    mac_style;
	tt_uint16    lowest_rec_ppem;
	
	tt_int16     font_direction;
	tt_int16     index_to_loc_format;
	tt_int16     glyph_data_format;
} TT_header;

/* TTAG_post */
typedef struct
{
	tt_uint32   format_type;
	tt_uint32   italic_angle;
	tt_int16    underline_position;
	tt_int16    underline_thickness;
	tt_uint32   is_fixed_pitch;
	tt_uint32   min_mem_type42;
	tt_uint32   max_mem_type42;
	tt_uint32   min_mem_type1;
	tt_uint32   max_mem_type1;
	
	/* field only present in format 2.0 and 2.5 */
	tt_uint16  num_glyphs;
	tt_uint16  glyphid_array[0];
	/* int8_t names[]; */
} TT_postscript;

/* TTAG_hhea/TTAG_vhea */
typedef struct
{
	tt_uint16    major_version;
	tt_uint16    minor_version;
	tt_int16     ascender;
	tt_int16     descender;
	tt_int16     line_gap;
	
	tt_uint16    advance_width_max;      /* advance width maximum */
	
	tt_int16     min_left_side_bearing;  /* minimum left-sb       */
	tt_int16     min_right_side_bearing; /* minimum right-sb      */
	tt_int16     xmax_extent;            /* xmax extents          */
	tt_int16     caret_slope_rise;
	tt_int16     caret_slope_run;
	tt_int16     caret_offset;
	
	tt_int16     reserved[4];
	
	tt_int16     metric_data_format;
	tt_uint16    number_of_metrics;
} TT_horiheader, TT_vertheader;

/* TTAG_maxp */
typedef struct
{
	tt_uint32  version;
	tt_uint16  num_glyphs;
	tt_uint16  max_points;
	tt_uint16  max_contours;
	tt_uint16  max_composite_points;
	tt_uint16  max_composite_contours;
	tt_uint16  max_zones;
	tt_uint16  max_twilight_points;
	tt_uint16  max_storage;
	tt_uint16  max_function_defs;
	tt_uint16  max_instruction_defs;
	tt_uint16  max_stack_elements;
	tt_uint16  max_sizeof_instructions;
	tt_uint16  max_component_elements;
	tt_uint16  max_component_depth;
} TT_maxprofile;

/* TTAG_glyf */
typedef struct {
	tt_int16  num_contours;
	tt_int16  xmin;
	tt_int16  ymin;
	tt_int16  xmax;
	tt_int16  ymax;
} TT_glyf;

/* Flags for Coordinates */
#define GLYF_FLAGS_ON_CURVE       0x01
#define GLYF_FLAGS_X_SHORT_VECTOR 0x02
#define GLYF_FLAGS_Y_SHORT_VECTOR 0x04
#define GLYF_FLAGS_REPEAT         0x08
#define GLYF_FLAGS_X_SAME         0x10
#define GLYF_FLAGS_Y_SAME         0x20

/* Flags for Composite Glyph */
#define GLYF_ARG_1_AND_2_ARE_WORDS    0x001
#define GLYF_ARGS_ARE_XY_VALUES       0x002
#define GLYF_ROUND_XY_TO_GRID         0x004
#define GLYF_WE_HAVE_A_SCALE          0x008
#define GLYF_NO_OVERLAP               0x010
#define GLYF_MORE_COMPONENT           0x020
#define GLYF_WE_HAVE_AN_X_AND_Y_SCALE 0x040
#define GLYF_WE_HAVE_A_TWO_BY_TWO     0x080
#define GLYF_WE_HAVE_INSTRUCTIONS     0x100
#define GLYF_USE_MY_METRICS           0x200
#define GLYF_OVERLAP_COMPOUND         0x400  /* from Apple's TTF specs */

typedef struct {
	tt_uint16  left;
	tt_uint16  right;
	tt_int16  value;
} TT_kernpair;

typedef struct {
	tt_uint16  n_pairs;
	tt_uint16  search_range;
	tt_uint16  entry_selector;
	tt_uint16  range_shift;
	TT_kernpair pair[0];
} TT_kernformat0;

typedef struct {
	tt_uint16  version;
	tt_uint16  length;
	tt_uint16  coverage;
	TT_kernformat0 kern;
} TT_kernsubtable;

/* TTAG_kern */
typedef struct {
	tt_uint16  version;
	tt_uint16  num_tables;
	/* TT_kernsubtable tables[]; */
} TT_kerntable;

typedef struct
{
	tt_uint16  platform_id;
	tt_uint16  encoding_id;
	tt_uint16  language_id;
	tt_uint16  name_id;
	tt_uint16  string_length;
	tt_offset16 string_offset;
} TT_name;

/* TTAG_name */
typedef struct
{
	tt_uint16  format;
	tt_uint16  num_name_records;
	tt_offset16 storage_offset;
	TT_name names[0];
} TT_nametable;

/* TTAG_PCLT */
typedef struct
{
	tt_uint16  major_version;
	tt_uint16  minor_version;
	tt_uint32  font_number;
	tt_uint16  pitch;
	tt_uint16  xheight;
	tt_uint16  style;
	tt_uint16  type_family;
	tt_uint16  capheight;
	tt_uint16  symbolset;
	char typeface[16];
	char character_complement[8];
	char filename[6];
	int8_t stroke_weight;
	int8_t width_type;
	uint8_t serif_style;
	uint8_t reserved;
} TT_PCLT;

typedef struct {
	tt_uint16  platform_id;
	tt_uint16  encoding_id;
	tt_uint32  offset;
} TT_encoding;

typedef struct {
	tt_uint16  version;
	tt_uint16  num_encodings;
	TT_encoding encoding[0];
} TT_cmap;

typedef struct {
	tt_uint32  start_charcode;
	tt_uint32  end_charcode;
	tt_uint32  start_glyphid;
} TT_sequential_map_group;

typedef struct {
	tt_uint32  start_charcode;
	tt_uint32  end_charcode;
	tt_uint32  glyphid;
} TT_constant_map_group;

typedef struct {
	tt_uint32  var_selector;
	tt_uint32  default_uvs_offset;
	tt_uint32  nondefault_uvs_offset;
} TT_variation_selector;

/* TTAG_cmap */
typedef union {
	tt_uint16  format;
	struct {
		tt_uint16  format;
		tt_uint16  length;
		tt_uint16  language;
		uint8_t glyphid_array[256];
	} format0;
	struct {
		tt_uint16  format;
		tt_uint16  length;
		tt_uint16  language;
		tt_uint16  subheader_keys[256];
		struct {
			tt_uint16  first_code;
			tt_uint16  entry_count;
			tt_int16  id_delta;
			tt_uint16  id_range_offset;
		} subheaders[0];
		/* tt_uint16  glyphid_array[]; */
	} format2;
	struct {
		tt_uint16  format;
		tt_uint16  length;
		tt_uint16  language;
		tt_uint16  segcount_x2;
		tt_uint16  search_range;
		tt_uint16  entry_selector;
		tt_uint16  range_shift;
		tt_uint16  end_code[0];
		/* tt_uint16  pad; */
		/* tt_uint16  start_code[seg_count]; */
		/* tt_int16  id_delta[seg_count]; */
		/* tt_uint16  id_range_offsets[seg_count]; */
		/* tt_uint16  glyphid_array[]; */
	} format4;
	struct {
		tt_uint16  format;
		tt_uint16  length;
		tt_uint16  language;
		tt_uint16  first_code;
		tt_uint16  entry_count;
		tt_uint16  glyphid_array[0];
	} format6;
	struct {
		tt_uint16  format;
		tt_uint16  reserved;
		tt_uint32  length;
		tt_uint32  language;
		uint8_t is32[8192];
		tt_uint32  num_groups;
		TT_sequential_map_group group[0];
	} format8;
	struct {
		tt_uint16  format;
		tt_uint16  reserved;
		tt_uint32  length;
		tt_uint32  language;
		tt_uint32  start_charcode;
		tt_uint32  num_chars;
		tt_uint16  glyphid_array[0];
	} format10;
	struct {
		tt_uint16  format;
		tt_uint16  reserved;
		tt_uint32  length;
		tt_uint32  language;
		tt_uint32  num_groups;
		TT_sequential_map_group group[0];
	} format12;
	struct {
		tt_uint16  format;
		tt_uint16  reserved;
		tt_uint32  length;
		tt_uint32  language;
		tt_uint32  num_groups;
		TT_constant_map_group group[0];
	} format13;
	struct {
		tt_uint16  format;
		tt_uint32  length;
		tt_uint32  num_var_selector_records;
		TT_variation_selector var_selector[0];
	} __attribute__((packed)) format14;
} TT_cmaptable;

typedef struct {
	tt_uint16  advance_width;
	tt_int16  lsb;
} TT_longhormetric;

/* TTAG_hmtx/TTAG_vmtx */
typedef struct {
	TT_longhormetric metrics[1];
	/* tt_int16  left_side_bearing[]; */
} TT_hmtx, TT_vmtx;

typedef struct _TT_tablerec {
	tt_uint32  tag;						/*        table type */
	tt_uint32  checksum;				/*    table checksum */
	tt_uint32  offset;					/* table file offset */
	tt_uint32  length;					/*      table length */
} TT_tablerec;
#define SIZEOF_TT_tablerec 16


/* TTAG_gasp */
typedef struct
{
	tt_uint16  max_PPEM;
	tt_uint16  flags;
} TT_gasprange;
#define TT_GASP_GRIDFIT             0x01
#define TT_GASP_DOGRAY              0x02
#define TT_GASP_SYMMETRIC_GRIDFIT   0x04
#define TT_GASP_SYMMETRIC_SMOOTHING 0x08

typedef struct
{
	tt_uint16  version;
	tt_uint16  num_ranges;
	TT_gasprange range[0];
} TT_gasp;


typedef struct {
	uint8_t pixel_size;
	uint8_t max_width;
	uint8_t width[0];
} TT_devicerecord;

/* TTAG_hdmx */
typedef struct {
	tt_uint16  version;
	tt_uint16  num_devices;
	tt_uint32  size_device_record;
	/* TT_devicerecord record[]; */
} TT_hdmx;

/* TTAG_LTSH */
typedef struct {
	tt_uint16  version;
	tt_uint16  num_glyphs;
	uint8_t y_pels[0];
} TT_ltsh;


typedef struct {
	uint8_t charset;
	uint8_t xratio;
	uint8_t ystartratio;
	uint8_t yendratio;
} TT_ratiorange;

typedef struct {
	tt_uint16  ypelheight;
	tt_int16  ymax;
	tt_int16  ymin;
} TT_vdmx_vtable;

typedef struct {
	tt_uint16  num_entries;
	uint8_t startsz;
	uint8_t endsz;
	TT_vdmx_vtable entry[0];
} TT_vdmx_group;

/* TTAG_VDMX */
typedef struct {
	tt_uint16  version;
	tt_uint16  num_recs;
	tt_uint16  num_ratios;
	TT_ratiorange range[0];
	/* tt_uint16  group_offsets[]; */
} TT_VDMX;

typedef struct {
	tt_uint16  lookup_order_offset;
	tt_uint16  required_feature_index;
	tt_uint16  feature_index_count;
	tt_uint16  feature_index[0];
} TT_langsystable;

typedef struct {
	tt_uint32  langsys_tag;
	tt_offset16 langsys_offset; /* to TT_langsystable */
} __attribute__((packed)) TT_langsysrecord;

typedef struct {
	tt_offset16 default_langsysoffset; /* to TT_langsystable */
	tt_uint16  lang_sys_count;
	TT_langsysrecord records[0];
} __attribute__((packed)) TT_scripttable;

typedef struct {
	tt_uint32  script_tag;
	tt_offset16 script_offset; /* to TT_scripttable */
} __attribute__((packed)) TT_scriptrecord;

typedef struct {
	tt_uint16  count;
	TT_scriptrecord records[0];
} __attribute__((packed)) TT_scriptlisttable;

typedef struct {
	tt_offset16 feature_params_offset; /* to TT_featureparams */
	tt_uint16  lookup_index_count;
	tt_uint16  lookup_index[0];
} TT_featuretable;

typedef struct {
	tt_uint32  feature_tag;
	tt_offset16 feature_offset; /* to TT_featuretable */
} __attribute__((packed)) TT_featurerecord;

typedef struct {
	tt_uint16  count;
	TT_featurerecord records[0];
} __attribute__((packed)) TT_featurelisttable;

typedef struct {
	tt_uint16  start_glyphid;
	tt_uint16  end_glyphid;
	tt_uint16  start_coverage_index;
} __attribute__((packed)) TT_rangerecord;

typedef union {
	tt_uint16  format;
	struct {
		tt_uint16  format;
		tt_uint16  glyph_count;
		tt_uint16  glyph_array[0];
	} coverage1;
	struct {
		tt_uint16  format;
		tt_uint16  range_count;
		TT_rangerecord record[0];
	} coverage2;
} __attribute__((packed)) TT_coverage;

typedef struct {
	tt_int16  xplacement;
	tt_int16  yplacement;
	tt_int16  xadvance;
	tt_int16  yadvance;
	tt_offset16 xplace_device_offset;
	tt_offset16 yplace_device_offset;
	tt_offset16 xadvance_device_offset;
	tt_offset16 yadvance_device_offset;
} __attribute__((packed)) TT_valuerecord;

typedef union {
	tt_uint16  format;
	struct {
		tt_uint16  format;
		tt_uint16  start_glyphid;
		tt_uint16  glyph_count;
		tt_uint16  class_value[0];
	} class1;
	struct {
		tt_uint16  format;
		tt_uint16  range_count;
		TT_rangerecord record[0];
	} class2;
} __attribute__((packed)) TT_classdef;

typedef struct {
	tt_uint16  second_glyph;
	TT_valuerecord value1;
	TT_valuerecord value2;
} __attribute__((packed)) TT_pairvaluerecord;

typedef struct {
	tt_uint16  count;
	TT_pairvaluerecord record[0];
} __attribute__((packed)) TT_pairset;

typedef struct {
	TT_valuerecord value1;
	TT_valuerecord value2;
} __attribute__((packed)) TT_class2record;

typedef struct {
	tt_uint16  base_count;
	tt_uint16  baseanchor_offset[0]; /* base_count * class_count */
} TT_basearray;

typedef struct {
	tt_uint16  mark_class;
	tt_uint16  markanchor_offset;
} __attribute__((packed)) TT_markrecord;

typedef struct {
	tt_uint16  mark_count;
	TT_markrecord record[0];
} __attribute__((packed)) TT_markarray;

typedef struct {
	tt_uint16  mark_count;
	tt_offset16 anchor_offset[0];
} __attribute__((packed)) TT_mark2array;

typedef struct {
	tt_offset16 entryanchor_offset;
	tt_offset16 exitanchor_offset;
} __attribute__((packed)) TT_entryexitrecord;

typedef struct {
	tt_uint16  component_count;
	tt_offset16 ligatureanchor_offset[0]; /* component_count * class_count */
} TT_ligatureattach;

typedef struct {
	tt_uint16  ligature_glyph;
	tt_uint16  component_count;
	tt_uint16  component_glyphid[0];
} __attribute__((packed)) TT_ligaturetable;

typedef struct {
	tt_uint16  ligature_count;
	tt_offset16 ligatureattach_offset[0];
} __attribute__((packed)) TT_ligaturearray;

typedef struct {
	tt_uint16  sequence_index;
	tt_uint16  lookuplist_index;
} TT_sequencelookuprecord;

typedef struct {
	tt_uint16  glyph_count;
	tt_uint16  seqlookup_count;
	tt_uint16  input_sequence[0];
	/* TT_sequencelookuprecord[] */
} TT_sequencerule;

typedef struct {
	tt_uint16  glyph_count;
	tt_uint16  substitute_glyphid[0];
} TT_sequence;

typedef struct {
	tt_uint16  glyph_count;
	tt_uint16  alternate_glyphid[0];
} TT_alternateset;

typedef struct {
	tt_uint16  seqrule_count;
	tt_offset16 seqrule_offset[0];
} __attribute__((packed)) TT_seqruleset;

typedef struct {
	tt_uint16  start_size;
	tt_uint16  end_size;
	tt_uint16  delta_format;
	tt_uint16  delta_value[0];
} TT_devicetable;

typedef union {
	tt_uint16  format;
	struct {
		tt_uint16  format;
		tt_int16  xcoordinate;
		tt_int16  ycoordinate;
	} anchor1;
	struct {
		tt_uint16  format;
		tt_int16  xcoordinate;
		tt_int16  ycoordinate;
		tt_uint16  anchorpoint;
	} anchor2;
	struct {
		tt_uint16  format;
		tt_int16  xcoordinate;
		tt_int16  ycoordinate;
		tt_offset16 xdeviceoffset;
		tt_offset16 ydeviceoffset;
	} anchor3;
} __attribute__((packed)) TT_anchor;

typedef union {
	tt_uint16  format;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  value_format;
		TT_valuerecord value;
	} single_adjustment1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  value_format;
		tt_uint16  value_count;
		TT_valuerecord value[0];
	} single_adjustment2;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_int16  delta_glyphid;
	} single_substitution1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  glyph_count;
		tt_uint16  substitute_glyphid[0];
	} single_substitution2;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  value_format1;
		tt_uint16  value_format2;
		tt_uint16  pairset_count;
		tt_offset16 pairset_offset[0];
	} pair_adjustment1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  value_format1;
		tt_uint16  value_format2;
		tt_offset16 classdef1_offset;
		tt_offset16 classdef2_offset;
		tt_uint16  class1_count;
		tt_uint16  class2_count;
		TT_valuerecord value[0];
	} pair_adjustment2;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  sequence_count;
		tt_uint16  sequence_offset[0];
	} multiple_substitution1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  alternateset_count;
		tt_uint16  alternateset_offset[0];
	} alternate_substitution1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  entry_exit_count;
		TT_entryexitrecord record[0];
	} cursive1;
	struct {
		tt_uint16  format;
		tt_offset16 markcoverage_offset;
		tt_offset16 basecoverage_offset;
		tt_uint16  class_count;
		tt_offset16 markarray_offset;
		tt_offset16 basearray_offset;
	} markbase1;
	struct {
		tt_uint16  format;
		tt_offset16 markcoverage_offset;
		tt_offset16 ligaturecoverage_offset;
		tt_uint16  class_count;
		tt_offset16 markarray_offset;
		tt_offset16 ligaturearray_offset;
	} marklig1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  ligatureset_count;
		tt_offset16 ligatureset_offset[0];
	} ligaturesubst1;
	struct {
		tt_uint16  format;
		tt_offset16 mark1coverage_offset;
		tt_offset16 mark2coverage_offset;
		tt_uint16  class_count;
		tt_offset16 mark1array_offset;
		tt_offset16 mark2array_offset;
	} markmark1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  seqruleset_count;
		tt_offset16 seqruleset_offset[0];
	} ctx1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_offset16 classdef_offset;
		tt_uint16  classset_count;
		tt_offset16 seqruleset_offset[0];
	} ctx2;
	struct {
		tt_uint16  format;
		tt_uint16  glyph_count;
		tt_uint16  seqlookup_count;
		tt_offset16 coverage_offset[0];
		/* TT_sequencelookuprecord[]; */
	} ctx3;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_uint16  seqruleset_count;
		tt_offset16 seqruleset_offset[0];
	} chn1;
	struct {
		tt_uint16  format;
		tt_offset16 coverage_offset;
		tt_offset16 backtraceclassdef_offset;
		tt_offset16 inputclassdef_offset;
		tt_offset16 lookaheadclassdef_offset;
		tt_uint16  seqruleset_count;
		tt_offset16 seqruleset_offset[0];
	} chn2;
	struct {
		tt_uint16  format;
		tt_uint16  backtrack_glyphcount;
		tt_offset16 backtrackcoverage_offset[0];
	} chn3;
	struct {
		tt_uint16  format;
		tt_uint16  coverage_offset;
		tt_uint16  backtrack_glyphcount;
		tt_offset16 backtrackcoverage_offset[0];
		/* tt_uint16  lookahead_glyphcount; */
		/* tt_offset16 lookaheadcoverage_offset[]; */
		/* tt_uint16  substitute_glyphcount; */
		/* tt_uint16  substitute_glyphid[]; */
	} rchn1;
	struct {
		tt_uint16  format;
		tt_uint16  lookup_type;
		tt_uint32  offset;
	} extension;
} __attribute__((packed)) TT_lookupsubtable;

typedef struct {
	tt_uint16  type;
	tt_uint16  flags;
	tt_uint16  subtable_count;
	tt_uint16  subtable_offset[0];
	/* tt_uint16  mark_filtering_set; */
} __attribute__((packed)) TT_lookuptable;

typedef struct {
	tt_uint16  count;
	tt_uint16  lookup_offset[0]; /* to TT_lookuptable */
} __attribute__((packed)) TT_lookuplisttable;


/* TTAG_GPOS, TTAG_GSUB */
typedef struct {
	tt_uint16  major_version;
	tt_uint16  minor_version;
	tt_uint16  scriptlist_offset;
	tt_uint16  featurelist_offset;
	tt_uint16  lookuplist_offset;
	/* only in 1.1: */
	tt_uint32  featurevariations_offset;
} __attribute__((packed)) TT_GPOS;
typedef TT_GPOS TT_GSUB;

/* TTAG_GDEF */
typedef struct {
	tt_uint16  major_version;
	tt_uint16  minor_version;
	tt_offset16 classdef_offset;
	tt_offset16 attachlist_offset;
	tt_offset16 ligaturelist_offset;
	tt_offset16 markattach_offset;
	/* only in 1.2 */
	tt_offset16 markglyphsets_offset;
	/* only in 1.3 */
	tt_offset16 itemvarstore_offset;
} __attribute__((packed)) TT_GDEF;

/* TTAG_OS2 */
typedef struct
{
	tt_uint16  version;					/* 0x0001 - more or 0xFFFF */
	tt_int16   xAvgCharWidth;
	tt_uint16  usWeightClass;
	tt_uint16  usWidthClass;
	tt_uint16  fsType;
	tt_int16   ySubscriptXSize;
	tt_int16   ySubscriptYSize;
	tt_int16   ySubscriptXOffset;
	tt_int16   ySubscriptYOffset;
	tt_int16   ySuperscriptXSize;
	tt_int16   ySuperscriptYSize;
	tt_int16   ySuperscriptXOffset;
	tt_int16   ySuperscriptYOffset;
	tt_int16   yStrikeoutSize;
	tt_int16   yStrikeoutPosition;
	tt_int16   sFamilyClass;

	uint8_t panose[10];

	tt_uint32  ulUnicodeRange1;			/* Bits 0-31   */
	tt_uint32  ulUnicodeRange2;			/* Bits 32-63  */
	tt_uint32  ulUnicodeRange3;			/* Bits 64-95  */
	tt_uint32  ulUnicodeRange4;			/* Bits 96-127 */

	tt_uint32  achVendID;

	tt_uint16  fsSelection;
	tt_uint16  usFirstCharIndex;
	tt_uint16  usLastCharIndex;
	tt_int16   sTypoAscender;
	tt_int16   sTypoDescender;
	tt_int16   sTypoLineGap;
	tt_uint16  usWinAscent;
	tt_uint16  usWinDescent;

	/* only version 1 and higher: */

	tt_uint32  ulCodePageRange1;			/* Bits 0-31   */
	tt_uint32  ulCodePageRange2;			/* Bits 32-63  */

	/* only version 2 and higher: */

	tt_int16   sxHeight;
	tt_int16   sCapHeight;
	tt_uint16  usDefaultChar;
	tt_uint16  usBreakChar;
	tt_uint16  usMaxContext;

	/* only version 5 and higher: */

	tt_uint16  usLowerOpticalPointSize;	/* in twips (1/20th points) */
	tt_uint16  usUpperOpticalPointSize;	/* in twips (1/20th points) */
} __attribute__((packed)) TT_OS2;

/* TTAG_FFTM */
typedef struct {
	tt_uint32  version;
	tt_uint32  timestamp[2];
	tt_uint32  created[2];
	tt_uint32  modified[2];
} TT_FFTM;

#define TTC_MAGIC TT_MAKE_TAG('t', 't', 'c', 'f')
typedef struct
{
	tt_uint32  tag;
	tt_uint16  major_version;
	tt_uint16  minor_version;
	tt_uint32  num_fonts;
	tt_uint32  font_offset[0];
} TTC_fileheader;

typedef struct _TT_fileheader {
	tt_uint32  sfnt_version;
	tt_uint16  num_tables;
	tt_uint16  search_range;
	tt_uint16  entry_selector;
	tt_uint16  range_shift;
#ifdef TT_FLAT_ACCESS
	TT_tablerec records[0];
#else
	TT_tablerec *records;
#endif
} TT_fileheader;
#define SIZEOF_TT_fileheader 12

#endif

