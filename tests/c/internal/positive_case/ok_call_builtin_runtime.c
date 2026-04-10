struct Pair {
  int x;
  double y;
};

int main(void) {
  unsigned int bits = 40u;
  unsigned long long wide = ((unsigned long long)bits << 33) | 8ull;
  float negf = -1.5f;
  double negd = -1.5;
  long double negld = -0.25L;
  __complex__ double z = 1.0 + 2.0i;
  __complex__ double z_conj = __builtin_conj(z);
  struct Pair pair = {1, 2.0};

  if (__builtin_ffs(5) != 1) return 1;
  if (__builtin_ctz(bits) != 3) return 2;
  if (__builtin_clz(bits) != 26) return 3;
  if (__builtin_popcount(bits) != 2) return 4;
  if (__builtin_parity(bits) != 0) return 5;
  if (__builtin_clrsb(-4) != 29) return 6;

  if (__builtin_ffsll((long long)wide) != 4) return 7;
  if (__builtin_ctzll((long long)wide) != 3) return 8;
  if (__builtin_clzll((long long)wide) != 25) return 9;
  if (__builtin_popcountll((long long)wide) != 3) return 10;
  if (__builtin_parityll((long long)wide) != 1) return 11;
  if (__builtin_clrsbll(-4ll) != 61) return 12;

  if (!__builtin_signbit(negd)) return 13;
  if (!__builtin_signbitl(negld)) return 14;
  if (__builtin_signbit(3.0)) return 15;

  if (__builtin_copysign(1.25, -2.0) != -1.25) return 16;
  if (__builtin_copysignf(1.25f, negf) != -1.25f) return 17;
  if (__builtin_copysignl(1.25L, -2.0L) != -1.25L) return 18;
  if (__builtin_fabs(-2.5) != 2.5) return 19;
  if (__builtin_fabsf(-2.5f) != 2.5f) return 20;
  if (__builtin_fabsl(-2.5L) != 2.5L) return 21;

  if (__builtin_classify_type((void*)0) != 5) return 22;
  if (__builtin_classify_type(3.0) != 8) return 23;
  if (__builtin_classify_type(pair) != 12) return 24;

  if (__real__ z_conj != 1.0) return 25;
  if (__imag__ z_conj != -2.0) return 26;

  return 0;
}
