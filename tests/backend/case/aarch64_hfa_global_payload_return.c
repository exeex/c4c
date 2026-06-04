struct HfaFloat2 {
  float first;
  float second;
};

struct HfaDouble2 {
  double first;
  double second;
};

struct HfaFloat2 global_return_float2 = {12.1f, 12.2f};
struct HfaDouble2 global_return_double2 = {22.1, 22.2};

struct HfaFloat2 return_float2_global(void) {
  return global_return_float2;
}

struct HfaDouble2 return_double2_global(void) {
  return global_return_double2;
}
