#include <ft2bld.h>
#include <ft2build.h>
#include <freetype/freetype.h>

#undef FTERRORS_H_
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST
#define FT_ERROR_END_LIST

static struct
{
	FT_Error err_code;
	const char *err_msg;
} const ft_errors[] = {
#include <freetype/fterrors.h>
};

FT_EXPORT(const char *) FT_Strerror(FT_Error code)
{
	size_t i;

	for (i = 0; i < (sizeof(ft_errors) / sizeof(ft_errors[0])); i++)
		if (ft_errors[i].err_code == code)
			return ft_errors[i].err_msg;
	return "unknown error";
}
