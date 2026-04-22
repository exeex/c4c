struct S1 {
  char x[1];
};

struct S2 {
  char x[2];
};

struct S3 {
  char x[3];
};

struct Hfa14 {
  float a;
  float b;
  float c;
  float d;
};

struct Hfa24 {
  double a;
  double b;
  double c;
  double d;
};

struct Hfa34 {
  long double a;
  long double b;
  long double c;
  long double d;
};

static int same_bytes(const char *lhs, const char *rhs, int count) {
  for (int i = 0; i < count; ++i) {
    if (lhs[i] != rhs[i]) {
      return 0;
    }
  }
  return 1;
}

static int fa4(struct S1 a,
               struct Hfa14 b,
               struct S2 c,
               struct Hfa24 d,
               struct S3 e,
               struct Hfa34 f) {
  int status = 0;
  status |= same_bytes(a.x, "0", 1) ? 0 : 1;
  status |= (b.a == 14.1f && b.d == 14.4f) ? 0 : 2;
  status |= same_bytes(c.x, "12", 2) ? 0 : 4;
  status |= (d.a == 24.1 && d.d == 24.4) ? 0 : 8;
  status |= same_bytes(e.x, "345", 3) ? 0 : 16;
  status |= (f.a == (long double)34.1 && f.d == (long double)34.4) ? 0 : 32;
  return status;
}

int main(void) {
  struct S1 s1 = {"0"};
  struct S2 s2 = {"12"};
  struct S3 s3 = {"345"};
  struct Hfa14 hfa14 = {14.1f, 14.2f, 14.3f, 14.4f};
  struct Hfa24 hfa24 = {24.1, 24.2, 24.3, 24.4};
  struct Hfa34 hfa34 = {(long double)34.1,
                        (long double)34.2,
                        (long double)34.3,
                        (long double)34.4};
  return fa4(s1, hfa14, s2, hfa24, s3, hfa34);
}
