#if defined(__APPLE__)
int main() {
  return 0;
}
#else
typedef __decltype(0.0bf16) bf16_gnu_t;
typedef decltype(1.0f) f32_std_t;

int main() {
  bf16_gnu_t x = 1.0bf16;
  f32_std_t y = 2.0f;
  return (x > 0.0f && y > 0.0f) ? 0 : 1;
}
#endif
