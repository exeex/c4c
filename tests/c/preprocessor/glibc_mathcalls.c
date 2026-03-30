// Reduced repro for glibc mathcalls helper macros.
// Mirrors /usr/include/x86_64-linux-gnu/bits/mathcalls-helper-functions.h
// with _Mdouble_ mapped to _Float128 so parser must accept fixed-width
// floating-point keywords inside macro-generated prototypes.

#define __MATHDECL_1(type, function, suffix, args) \
  extern type function args __attribute__((__nothrow__ , __leaf__))
#define __MATHDECL_ALIAS(type, function, suffix, args, alias) \
  __MATHDECL_1(type, function, suffix, args)

#define _Mdouble_ _Float128

__MATHDECL_ALIAS (int, __fpclassify,, (_Mdouble_ __value), fpclassify)
     __attribute__ ((__const__));
__MATHDECL_ALIAS (int, __signbit,, (_Mdouble_ __value), signbit)
     __attribute__ ((__const__));
__MATHDECL_ALIAS (int, __isinf,, (_Mdouble_ __value), isinf)
  __attribute__ ((__const__));
__MATHDECL_ALIAS (int, __finite,, (_Mdouble_ __value), finite)
  __attribute__ ((__const__));
__MATHDECL_ALIAS (int, __isnan,, (_Mdouble_ __value), isnan)
  __attribute__ ((__const__));
__MATHDECL_ALIAS (int, __iseqsig,, (_Mdouble_ __x, _Mdouble_ __y), iseqsig);
__MATHDECL_ALIAS (int, __issignaling,, (_Mdouble_ __value), issignaling)
     __attribute__ ((__const__));

#undef _Mdouble_
#undef __MATHDECL_ALIAS
#undef __MATHDECL_1

int force_link(void) {
  return 0;
}
