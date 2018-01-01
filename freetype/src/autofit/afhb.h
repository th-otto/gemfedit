#ifdef FT_CONFIG_OPTION_USE_HARFBUZZ

#include <hb.h>
#include <hb-ot.h>
#include <hb-ft.h>

#undef HB_VERSION_ATLEAST
#define HB_VERSION_ATLEAST(major,minor,micro) \
	((major)*10000+(minor)*100+(micro) <= \
	 HB_VERSION_MAJOR*10000+HB_VERSION_MINOR*100+HB_VERSION_MICRO)

#else

#undef HB_VERSION_ATLEAST
#define HB_VERSION_ATLEAST(major,minor,micro) 0

#endif
