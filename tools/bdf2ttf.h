/* vim:set ts=8 sts=4 sw=4 tw=0: */
/*
 * bdf2ttf.h - BDF to TTF converter.
 *
 * Written By:	MURAOKA Taro <koron.kaoriya@gmail.com>
 * Last Change: 03-Jul-2016.
 */
#ifndef BDF2TTF_H
#define BDF2TTF_H

#ifdef __cplusplus
extern "C" {
#endif

extern const char	*g_copyright;
extern const char	*g_copyright_cp;
extern const char	*g_fontname;
extern const char	*g_fontname_cp;
extern const char	*g_version;
extern const char	*g_version_cp;
extern const char	*g_trademark;
extern const char	*g_trademark_cp;

int emCalc(int pix, int base);
int write_ttf(bdf2_t *font, const char *ttfname);

#ifdef __cplusplus
}
#endif

#endif /* BDF2TTF_H */
