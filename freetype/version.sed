#! /usr/bin/sed -nf

s/^#define  *FREETYPE_MAJOR  *\([0-9]*\).*$/freetype_major="\1" ;/p
s/^#define  *FREETYPE_MINOR  *\([0-9]*\).*$/freetype_minor=".\1" ;/p
s/^#define  *FREETYPE_PATCH  *\([0-9]*\).*$/freetype_patch=".\1" ;/p
