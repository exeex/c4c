struct HfaF128_2 {
  long double first;
  long double second;
};

extern void take_f128_2(struct HfaF128_2 value);

struct HfaF128_2 global_f128_2 = {(long double)32.1, (long double)32.2};

void call_f128_hfa_globals(void) {
  take_f128_2(global_f128_2);
}
