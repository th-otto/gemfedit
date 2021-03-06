/***************************************************************************/
/*                                                                         */
/*  cffdrivr.h                                                             */
/*                                                                         */
/*    High-level OpenType driver interface (specification).                */
/*                                                                         */
/*  Copyright 1996-2017 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef CFFDRIVER_H_
#define CFFDRIVER_H_


#include <ft2build.h>
#include <freetype/internal/ftdriver.h>


FT_BEGIN_HEADER

FT_CALLBACK_TABLE const FT_Driver_ClassRec cff_driver_class;

FT_END_HEADER

#endif /* CFFDRIVER_H_ */
