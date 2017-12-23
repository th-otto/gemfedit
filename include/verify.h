#ifndef _GL_VERIFY_H
#define _GL_VERIFY_H


/* Concatenate two preprocessor tokens.  */
#define _GL_CONCAT(x, y) _GL_CONCAT0 (x, y)
#define _GL_CONCAT0(x, y) x##y

/* _GL_COUNTER is an integer, preferably one that changes each time we
   use it.  Use __COUNTER__ if it works, falling back on __LINE__
   otherwise.  __LINE__ isn't perfect, but it's better than a
   constant.  */
#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
# define _GL_COUNTER __COUNTER__
#else
# define _GL_COUNTER __LINE__
#endif

/* Generate a symbol with the given prefix, making it unique if
   possible.  */
#define _GL_GENSYM(prefix) _GL_CONCAT (prefix, _GL_COUNTER)

/* Verify requirement R at compile-time, as a declaration without a
   trailing ';'.  If R is false, fail at compile-time, preferably
   with a diagnostic that includes the string-literal DIAGNOSTIC.

   Unfortunately, unlike C11, this implementation must appear as an
   ordinary declaration, and cannot appear inside struct { ... }.  */

# define _GL_VERIFY(R)                    \
    extern int (*_GL_GENSYM (_gl_verify_function) (void))         \
      [((R) ? 1 : -1)]

/* Verify requirement R at compile-time, as a declaration without a
   trailing ';'.  */

#define verify(R) _GL_VERIFY (R)

#endif
