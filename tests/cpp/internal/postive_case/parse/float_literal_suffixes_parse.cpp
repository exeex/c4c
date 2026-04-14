// Parse-only coverage for C++23 fixed-width floating literal suffix spellings.
int main() {
  float a = 0.0f16;
  float b = 0.0F16;
  float c = 0.0f32;
  float d = 0.0F32;
  double e = 0.0f64;
  double f = 0.0F64;
  long double g = 0.0f128;
  long double h = 0.0F128;
  float i = 0.0bf16;
  float j = 0.0BF16;
  return (a == b && c == d && e == f && g == h && i == j) ? 0 : 1;
}
