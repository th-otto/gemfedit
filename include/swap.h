#if defined(__PUREC__) || defined(__mc68000__)
#define WORDS_BIGENDIAN 1
#endif

#ifdef WORDS_BIGENDIAN
static const int HOST_BIG = 1;
#else
static const int HOST_BIG = 0;
#endif


#undef INLINE
#ifdef __PUREC__
#define INLINE
#else
#define INLINE __inline
#endif

typedef unsigned char UB;
typedef   signed char B;
typedef unsigned short UW;
typedef   signed short W;
typedef unsigned long UL;
typedef   signed long L;

static INLINE B *TO_B(void *s) { return (B *)s; }
static INLINE UB *TO_UB(void *s) { return (UB *)s; }
static INLINE W *TO_W(void *s) { return (W *)s; }
static INLINE UW *TO_UW(void *s) { return (UW *)s; }
static INLINE L *TO_L(void *s) { return (L *)s; }
static INLINE UL *TO_UL(void *s) { return (UL *)s; }

static INLINE const B *TO_B_C(const void *s) { return (const B *)s; }
static INLINE const UB *TO_UB_C(const void *s) { return (const UB *)s; }
static INLINE const W *TO_W_C(const void *s) { return (const W *)s; }
static INLINE const UW *TO_UW_C(const void *s) { return (const UW *)s; }
static INLINE const L *TO_L_C(const void *s) { return (const L *)s; }
static INLINE const UL *TO_UL_C(const void *s) { return (const UL *)s; }

/* Load/Store primitives (without address checking) */
#define LOAD_B(_s) (*(TO_B_C(_s)))
#define LOAD_UB(_s) (*(TO_UB_C(_s)))
#define LOAD_W(_s) (*(TO_W_C(_s)))
#define LOAD_UW(_s) (*(TO_UW_C(_s)))
#define LOAD_L(_s) (*(TO_L_C(_s)))
#define LOAD_UL(_s) (*(TO_UL_C(_s)))

#define STORE_B(_d,_v) *(TO_B(_d)) = _v
#define STORE_UB(_d,_v)	*(TO_UB(_d)) = _v
#define STORE_W(_d,_v) *(TO_W(_d)) = _v
#define STORE_UW(_d,_v) *(TO_UW(_d)) = _v
#define STORE_L(_d,_v) *(TO_L(_d)) = _v
#define STORE_UL(_d,_v) *(TO_UL(_d)) = _v

static INLINE UW swap_w(UW x)
{
	return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

static INLINE UL swap_l(UL x)
{
	return ((((x) & 0xff000000UL) >> 24) | (((x) & 0x00ff0000UL) >>  8) |
      (((x) & 0x0000ff00UL) <<  8) | (((x) & 0x000000ffUL) << 24));
}

#define SWAP_W(s) s = swap_w(s)
#define SWAP_L(s) s = swap_l(s)

#ifdef WORDS_BIGENDIAN

#define LM_W(s) LOAD_W(s)
#define LM_UW(s) LOAD_UW(s)
#define LM_L(s) LOAD_L(s)
#define LM_UL(s) LOAD_UL(s)
#define SM_W(d, v) STORE_W(d, v)
#define SM_UW(d, v) STORE_UW(d, v)
#define SM_L(d, v) STORE_L(d, v)
#define SM_UL(d, v) STORE_UL(d, v)

#else

#define LM_W(s) ((W)swap_w(LOAD_W(s)))
#define LM_UW(s) ((UW)swap_w(LOAD_UW(s)))
#define LM_L(s) ((L)swap_l(LOAD_L(s)))
#define LM_UL(s) ((UL)swap_l(LOAD_UL(s)))
#define SM_W(d, v) STORE_W(d, swap_w(v))
#define SM_UW(d, v) STORE_UW(d, swap_w(v))
#define SM_L(d, v) STORE_L(d, swap_l(v))
#define SM_UL(d, v) STORE_UL(d, swap_l(v))

#endif /* WORDS_BIGENDIAN */

