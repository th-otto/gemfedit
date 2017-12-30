#ifndef __VERIFY_H__
#define __VERIFY_H__

/* Concatenate two preprocessor tokens. */
#ifndef __CONCAT
#define __CONCAT(x, y) __CONCAT0(x, y)
#define __CONCAT0(x, y) x##y
#endif

#if defined __COUNTER__ && __COUNTER__ != __COUNTER__
# define _GL_COUNTER __COUNTER__
#else
# define _GL_COUNTER __LINE__
#endif

/* Generate a symbol with the given prefix, making it unique if
   possible.  */
#define __GENSYM(prefix) __CONCAT(prefix, _GL_COUNTER)

/* Verify requirement R at compile-time, as a declaration without a
   trailing ';'.  */

#define verify(R)                    \
    extern int (*__GENSYM (__verify_function) (void)) [((R) ? 1 : -1)]

#endif
