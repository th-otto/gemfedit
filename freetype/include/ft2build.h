/***************************************************************************/
/*                                                                         */
/*  ft2build.h                                                             */
/*                                                                         */
/*    FreeType 2 build and setup macros.                                   */
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


/*************************************************************************/
/*                                                                       */
/* This is the `entry point' for FreeType header file inclusions.  It is */
/* the only header file which should be included directly; all other     */
/* FreeType header files should be accessed with macro names (after      */
/* including `ft2build.h').                                              */
/*                                                                       */
/* A typical example is                                                  */
/*                                                                       */
/*   #include <ft2build.h>                                               */
/*   #include FT_FREETYPE_H                                              */
/*                                                                       */
/*************************************************************************/


#ifndef __FT2BUILD_H__
#define __FT2BUILD_H__

#ifdef __PUREC__

#pragma warn -stu

#ifndef ANONYMOUS_STRUCT_DUMMY
#define ANONYMOUS_STRUCT_DUMMY(x) struct x { int dummy; };
#endif

#else

#ifndef ANONYMOUS_STRUCT_DUMMY
#define ANONYMOUS_STRUCT_DUMMY(x)
#endif

#endif

#define FT2_STATIC 1

/* `<prefix>/include/freetype2' must be in your current inclusion path */
#include <freetype/config/ftheader.h>

#endif /* FT2BUILD_H_ */
