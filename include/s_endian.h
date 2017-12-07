#ifndef __S_ENDIAN_H__
#define __S_ENDIAN_H__ 1

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    4321
#define LITTLE_ENDIAN 1234
#define PDP_ENDIAN    3412
#endif

#ifndef BYTE_ORDER
#    ifdef __BYTE_ORDER
#      define BYTE_ORDER __BYTE_ORDER
#    else
#      ifdef _BYTE_ORDER
#        define BYTE_ORDER _BYTE_ORDER
#      endif
#    endif
#endif

#ifndef BYTE_ORDER

#  ifdef __mc68000__
#    define BYTE_ORDER BIG_ENDIAN
#  endif
#  ifdef __mc68k__
#    define BYTE_ORDER BIG_ENDIAN
#  endif
#  ifdef m68k
#    define BYTE_ORDER BIG_ENDIAN
#  endif
#  ifdef __PUREC__
#    define BYTE_ORDER BIG_ENDIAN
#  endif

#  ifdef __i386__
#    define BYTE_ORDER LITTLE_ENDIAN
#  endif
#  ifdef __i486__
#    define BYTE_ORDER LITTLE_ENDIAN
#  endif
#  ifdef __i586__
#    define BYTE_ORDER LITTLE_ENDIAN
#  endif
#  ifdef __x86_64__
#    define BYTE_ORDER LITTLE_ENDIAN
#  endif

#endif

#if BYTE_ORDER == BIG_ENDIAN
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

typedef uint8_t UB;
typedef int8_t B;
typedef uint16_t UW;
typedef int16_t W;
typedef uint32_t UL;
typedef int32_t L;

typedef int gboolean;
#ifndef FALSE
#define FALSE 0
#define TRUE  1
#endif

#undef __bitwise
#undef __force
#ifdef __CHECKER__
#define __bitwise		__attribute__((bitwise))
#define __force			__attribute__((force))
#else
#define __bitwise
#define __force
#endif

typedef uint16_t	__bitwise	uintle16_t;
typedef uint32_t	__bitwise	uintle32_t;
typedef uint16_t	__bitwise	uintbe16_t;
typedef uint32_t	__bitwise	uintbe32_t;
#ifndef __PUREC__
typedef uint64_t	__bitwise	uintle64_t;
typedef uint64_t	__bitwise	uintbe64_t;
#endif

#undef cpu_to_le64
#undef le64_to_cpu
#undef cpu_to_le32
#undef le32_to_cpu
#undef cpu_to_le16
#undef le16_to_cpu


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

static INLINE uint16_t cpu_swab16(uint16_t x)
{
	return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

static INLINE UL cpu_swab32(UL x)
{
	return ((((x) & 0xff000000UL) >> 24) | (((x) & 0x00ff0000UL) >>  8) |
      (((x) & 0x0000ff00UL) <<  8) | (((x) & 0x000000ffUL) << 24));
}

#define SWAP_W(s) STORE_UW(s, ((UW)cpu_swab16(LOAD_UW(s))))
#define SWAP_L(s) STORE_UL(s, ((UL)cpu_swab32(LOAD_UL(s))))

#if BYTE_ORDER == BIG_ENDIAN

#define cpu_to_le64(x) ((__force uintle64_t)cpu_swab64((uint64_t)(x)))
#define le64_to_cpu(x) cpu_swab64((__force uint64_t)(uintle64_t)(x))
#define cpu_to_le32(x) ((__force uintle32_t)cpu_swab32((uint32_t)(x)))
#define le32_to_cpu(x) cpu_swab32((__force uint32_t)(uintle32_t)(x))
#define cpu_to_le16(x) ((__force uintle16_t)cpu_swab16((uint16_t)(x)))
#define le16_to_cpu(x) cpu_swab16((__force uint16_t)(uintle16_t)(x))

#define cpu_to_be64(x) ((__force uintbe64_t)(uint64_t)(x))
#define be64_to_cpu(x) ((__force uint64_t)(uintbe64_t)(x))
#define cpu_to_be32(x) ((__force uintbe32_t)(uint32_t)(x))
#define be32_to_cpu(x) ((__force uint32_t)(uintbe32_t)(x))
#define cpu_to_be16(x) ((__force uintbe16_t)(uint16_t)(x))
#define be16_to_cpu(x) ((__force uint16_t)(uintbe16_t)(x))

#define LM_W(s) LOAD_W(s)
#define LM_UW(s) LOAD_UW(s)
#define LM_L(s) LOAD_L(s)
#define LM_UL(s) LOAD_UL(s)
#define SM_W(d, v) STORE_W(d, v)
#define SM_UW(d, v) STORE_UW(d, v)
#define SM_L(d, v) STORE_L(d, v)
#define SM_UL(d, v) STORE_UL(d, v)

#else

#define cpu_to_le64(x) ((__force uintle64_t)(uint64_t)(x))
#define le64_to_cpu(x) ((__force uint64_t)(uintle64_t)(x))
#define cpu_to_le32(x) ((__force uintle32_t)(uint32_t)(x))
#define le32_to_cpu(x) ((__force uint32_t)(uintle32_t)(x))
#define cpu_to_le16(x) ((__force uintle16_t)(uint16_t)(x))
#define le16_to_cpu(x) ((__force uint16_t)(uintle16_t)(x))

#define cpu_to_be64(x) ((__force uintbe64_t)cpu_swab64((uint64_t)(x)))
#define be64_to_cpu(x) cpu_swab64((__force uint64_t)(uintbe64_t)(x))
#define cpu_to_be32(x) ((__force uintbe32_t)cpu_swab32((uint32_t)(x)))
#define be32_to_cpu(x) cpu_swab32((__force uint32_t)(uintbe32_t)(x))
#define cpu_to_be16(x) ((__force uintbe16_t)cpu_swab16((uint16_t)(x)))
#define be16_to_cpu(x) cpu_swab16((__force uint16_t)(uintbe16_t)(x))

#define LM_W(s) ((W)be16_to_cpu(LOAD_W(s)))
#define LM_UW(s) ((UW)be16_to_cpu(LOAD_UW(s)))
#define LM_L(s) ((L)be32_to_cpu(LOAD_L(s)))
#define LM_UL(s) ((UL)be32_to_cpu(LOAD_UL(s)))
#define SM_W(d, v) STORE_W(d, cpu_to_be16(v))
#define SM_UW(d, v) STORE_UW(d, cpu_to_be16(v))
#define SM_L(d, v) STORE_L(d, cpu_to_be32(v))
#define SM_UL(d, v) STORE_UL(d, cpu_to_be32(v))

#endif /* BYTE_ORDER == BIG_ENDIAN */

#endif /* __S_ENDIAN_H__ */
