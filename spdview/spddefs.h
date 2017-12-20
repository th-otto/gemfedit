#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef __GNUC_PREREQ
# ifdef __GNUC__
#   define __GNUC_PREREQ(maj, min) ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#   define __GNUC_PREREQ(maj, min) 0
# endif
#endif

#ifndef G_GNUC_EXTENSION
#if __GNUC_PREREQ (2,8)
#define G_GNUC_EXTENSION __extension__
#else
#define G_GNUC_EXTENSION
#endif
#endif

/* Define G_VA_COPY() to do the right thing for copying va_list variables.
 * config.h may have already defined G_VA_COPY as va_copy or __va_copy.
 */
#if !defined (G_VA_COPY)
#  if defined (__GNUC__)
#    define G_VA_COPY(ap1, ap2)	va_copy(ap1, ap2)
#  elif defined (G_VA_COPY_AS_ARRAY)
#    define G_VA_COPY(ap1, ap2)	  memmove ((ap1), (ap2), sizeof (va_list))
#  else /* va_list is a pointer */
#    define G_VA_COPY(ap1, ap2)	  ((ap1) = (ap2))
#  endif /* va_list is a pointer */
#endif /* !G_VA_COPY */


typedef int gboolean;
typedef uint32_t gunichar;
typedef unsigned short gunichar2;
typedef void *gpointer;
typedef const void *gconstpointer;
G_GNUC_EXTENSION typedef signed long long gint64;
G_GNUC_EXTENSION typedef unsigned long long guint64;
typedef size_t gsize;
#if defined(_WIN64) || defined(__WIN64__) || defined(_M_X64) || defined(_M_AMD64)
typedef gint64 gssize;
#else
typedef long gssize;
#endif

#ifndef FALSE
#define FALSE   0                             /* Function FALSE value        */
#define TRUE    1                             /* Function TRUE  value        */
#endif

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define G_GNUC_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#define g_malloc(n) malloc(n)
#define g_try_malloc(n) malloc(n)
#define g_calloc(n, s) calloc(n, s)
#define g_malloc0(n) calloc(n, 1)
#define g_realloc(ptr, s) realloc(ptr, s)
#define g_free free

#undef g_ascii_isspace
#define g_ascii_isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n' || (c) == '\f' || (c) == '\v')
#undef g_ascii_isdigit
#define g_ascii_isdigit(c) ((c) >= '0' && (c) <= '9')
#undef g_ascii_isxdigit
#define g_ascii_isxdigit(c) (g_ascii_isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#undef g_ascii_isalpha
#define g_ascii_isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#undef g_ascii_isalnum
#define g_ascii_isalnum(c) ((c) == '_' || g_ascii_isalpha((c)) || g_ascii_isdigit((c)))
#undef g_ascii_toupper
#define g_ascii_toupper(c) toupper(c)
int g_ascii_strcasecmp(const char *s1, const char *s2);
int g_ascii_strncasecmp(const char *s1, const char *s2, size_t n);

#define STR0TERM ((size_t)-1)

#undef g_new
#define g_new(t, n) ((t *)g_malloc(sizeof(t) * (n)))
#undef g_renew
#define g_renew(t, p, n) ((t *)g_realloc(p, sizeof(t) * (n)))
#undef g_new0
#define g_new0(t, n) ((t *)g_calloc((n), sizeof(t)))

char *g_strdup_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
char *g_strdup_vprintf(const char *format, va_list args);

char *g_strdup(const char *);
char *g_strndup(const char *, size_t len);
char *g_strconcat(const char *, ...);
char **g_strsplit(const char *string, const char *delimiter, int max_tokens);
char *g_strjoinv(const char *separator, char **str_array);
char *g_stpcpy(char *dest, const char *src);

char *g_build_filename(const char *, ...);
char *g_get_current_dir(void);
gboolean g_path_is_absolute(const char *path);
char *g_strchomp(char *str);
char *g_strchug(char *str);
void g_strfreev(char **str_array);
unsigned int g_strv_length(char **str_array);

struct _GString
{
  char  *str;
  gsize len;
  gsize allocated_len;
};
typedef struct _GString GString;
GString *g_string_insert_c(GString *string, gssize pos, char c);
GString *g_string_append_c(GString *string, char c);
GString *g_string_sized_new(gsize dfl_size);
GString *g_string_new(const char *init);
GString *g_string_insert_len(GString *string, gssize pos, const char *val, gssize len);
GString *g_string_append_len(GString * string, const char *val, gssize len);
GString *g_string_append(GString *string, const char *val);
char *g_string_free(GString *string, gboolean free_segment);
void g_string_append_vprintf(GString *string, const char *format, va_list args) __attribute__((format(printf, 2, 0)));
void g_string_append_printf(GString *string, const char *format, ...) __attribute__((format(printf, 2, 3)));
GString *g_string_truncate(GString *string, gsize len);
GString *g_string_set_size(GString *string, gsize len);


#define fixnull(str) ((str) != NULL ? (str) : "")
#define printnull(str) ((str) != NULL ? (const char *)(str) : "(nil)")
#define empty(str) ((str) == NULL || *(str) == '\0')

const char *xbasename(const char *path);
char *spd_path_get_dirname(const char *path);
char *spd_uri_unescape_string(const char *escaped_string, const char *illegal_characters);
char *spd_uri_unescape_segment(const char *escaped_string, const char *escaped_string_end, const char *illegal_characters);
