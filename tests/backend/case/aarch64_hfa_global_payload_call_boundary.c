struct HfaFloat2 {
  float first;
  float second;
};

struct HfaDouble2 {
  double first;
  double second;
};

extern void take_float2(struct HfaFloat2 value);
extern void take_double2(struct HfaDouble2 value);

struct HfaFloat2 global_float2 = {12.1f, 12.2f};
struct HfaDouble2 global_double2 = {22.1, 22.2};

void call_hfa_globals(void) {
  take_float2(global_float2);
  take_double2(global_double2);
}
