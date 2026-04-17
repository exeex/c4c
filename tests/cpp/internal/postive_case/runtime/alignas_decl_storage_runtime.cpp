// Runtime regression: alignas(...) should apply to globals, locals, and fields.

constexpr unsigned kAlign = 32;

alignas(kAlign) int g_aligned = 7;

struct Holder {
  char pad;
  alignas(kAlign) int member;
};

int main() {
  alignas(kAlign) int local = 11;
  Holder h;
  h.member = 13;

  if (((unsigned long)&g_aligned) % kAlign != 0)
    return 1;
  if (((unsigned long)&local) % kAlign != 0)
    return 2;
  if (alignof(local) != kAlign)
    return 3;
  if (__builtin_offsetof(Holder, member) % kAlign != 0)
    return 4;
  if (alignof(Holder) < kAlign)
    return 5;
  if (((unsigned long)&h.member) % kAlign != 0)
    return 6;
  return 0;
}
