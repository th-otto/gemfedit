/***************************************************************************/
/*                                                                         */
/*  sfntpic.h                                                              */
/*                                                                         */
/*    The FreeType position independent code services for sfnt module.     */
/*                                                                         */
/*  Copyright 2009-2017 by                                                 */
/*  Oran Agra and Mickey Gabel.                                            */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef SFNTPIC_H_
#define SFNTPIC_H_


#include <freetype/internal/ftpic.h>


#ifndef FT_CONFIG_OPTION_PIC

#define SFNT_SERVICES_GET            sfnt_services
#define SFNT_SERVICE_GLYPH_DICT_GET  sfnt_service_glyph_dict
#define SFNT_SERVICE_PS_NAME_GET     sfnt_service_ps_name
#define TT_SERVICE_CMAP_INFO_GET     tt_service_get_cmap_info
#define TT_CMAP_CLASSES_GET          tt_cmap_classes
#define SFNT_SERVICE_SFNT_TABLE_GET  sfnt_service_sfnt_table
#define SFNT_SERVICE_BDF_GET         sfnt_service_bdf
#define SFNT_INTERFACE_GET           sfnt_interface

#else /* FT_CONFIG_OPTION_PIC */

  /* some include files required for members of sfntModulePIC */
#include <freetype/internal/services/svgldict.h>
#include <freetype/internal/services/svpostnm.h>
#include <freetype/internal/services/svsfnt.h>
#include <freetype/internal/services/svttcmap.h>

#ifdef TT_CONFIG_OPTION_BDF
#include "ttbdf.h"
#include <freetype/internal/services/svbdf.h>
#endif

#include <freetype/internal/ftdebug.h>
#include <freetype/internal/ftstream.h>
#include <freetype/internal/sfnt.h>
#include "ttcmap.h"


FT_BEGIN_HEADER

  typedef struct  sfntModulePIC_
  {
    FT_ServiceDescRec*        sfnt_services;
    FT_Service_GlyphDictRec   sfnt_service_glyph_dict;
    FT_Service_PsFontNameRec  sfnt_service_ps_name;
    FT_Service_TTCMapsRec     tt_service_get_cmap_info;
    TT_CMap_Class*            tt_cmap_classes;
    FT_Service_SFNT_TableRec  sfnt_service_sfnt_table;
#ifdef TT_CONFIG_OPTION_BDF
    FT_Service_BDFRec         sfnt_service_bdf;
#endif
    SFNT_Interface            sfnt_interface;

  } sfntModulePIC;


#define GET_PIC( lib )                                      \
          ( (sfntModulePIC*)( (lib)->pic_container.sfnt ) )

#define SFNT_SERVICES_GET                       \
          ( GET_PIC( library )->sfnt_services )
#define SFNT_SERVICE_GLYPH_DICT_GET                       \
          ( GET_PIC( library )->sfnt_service_glyph_dict )
#define SFNT_SERVICE_PS_NAME_GET                       \
          ( GET_PIC( library )->sfnt_service_ps_name )
#define TT_SERVICE_CMAP_INFO_GET                           \
          ( GET_PIC( library )->tt_service_get_cmap_info )
#define TT_CMAP_CLASSES_GET                       \
          ( GET_PIC( library )->tt_cmap_classes )
#define SFNT_SERVICE_SFNT_TABLE_GET                       \
          ( GET_PIC( library )->sfnt_service_sfnt_table )
#define SFNT_SERVICE_BDF_GET                       \
          ( GET_PIC( library )->sfnt_service_bdf )
#define SFNT_INTERFACE_GET                       \
          ( GET_PIC( library )->sfnt_interface )


  /* see sfntpic.c for the implementation */
  void
  sfnt_module_class_pic_free( FT_Library  library );

  FT_Error
  sfnt_module_class_pic_init( FT_Library  library );


FT_END_HEADER

#endif /* FT_CONFIG_OPTION_PIC */

  /* */

#endif /* SFNTPIC_H_ */


/* END */