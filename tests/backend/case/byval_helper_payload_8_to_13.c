struct S8 {
  char x[8];
};

struct S9 {
  char x[9];
};

struct S10 {
  char x[10];
};

struct S11 {
  char x[11];
};

struct S12 {
  char x[12];
};

struct S13 {
  char x[13];
};

static int same_bytes(const char *lhs, const char *rhs, int count) {
  for (int i = 0; i < count; ++i) {
    if (lhs[i] != rhs[i]) {
      return 0;
    }
  }
  return 1;
}

static int fa1(struct S8 a,
               struct S9 b,
               struct S10 c,
               struct S11 d,
               struct S12 e,
               struct S13 f) {
  int status = 0;
  status |= same_bytes(a.x, "stuvwxyz", 8) ? 0 : 1;
  status |= same_bytes(b.x, "ABCDEFGHI", 9) ? 0 : 2;
  status |= same_bytes(c.x, "JKLMNOPQRS", 10) ? 0 : 4;
  status |= same_bytes(d.x, "TUVWXYZ0123", 11) ? 0 : 8;
  status |= same_bytes(e.x, "456789abcdef", 12) ? 0 : 16;
  status |= same_bytes(f.x, "ghijklmnopqrs", 13) ? 0 : 32;
  return status;
}

int main(void) {
  struct S8 s8 = {"stuvwxyz"};
  struct S9 s9 = {"ABCDEFGHI"};
  struct S10 s10 = {"JKLMNOPQRS"};
  struct S11 s11 = {"TUVWXYZ0123"};
  struct S12 s12 = {"456789abcdef"};
  struct S13 s13 = {"ghijklmnopqrs"};
  return fa1(s8, s9, s10, s11, s12, s13);
}
