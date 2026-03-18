// Test #pragma pack(N), pack(), pack(push,N), pack(pop)
extern void abort(void);

// Default alignment
struct Default {
  char a;
  int b;
  short c;
};

#pragma pack(1)
struct Packed1 {
  char a;
  int b;
  short c;
};
#pragma pack()

// Back to default
struct Default2 {
  char a;
  int b;
  short c;
};

#pragma pack(push, 2)
struct Packed2 {
  char a;
  int b;
  short c;
};
#pragma pack(pop)

// Back to default after pop
struct Default3 {
  char a;
  int b;
  short c;
};

// __attribute__((packed)) on struct
struct __attribute__((packed)) AttrPacked {
  char a;
  int b;
  short c;
};

int main(void) {
  // Default: sizeof = 12 (char + 3 pad + int + short + 2 pad)
  if (sizeof(struct Default) != 12) abort();
  if (sizeof(struct Default2) != 12) abort();
  if (sizeof(struct Default3) != 12) abort();

  // pack(1): sizeof = 7 (no padding)
  if (sizeof(struct Packed1) != 7) abort();

  // pack(2): sizeof = 8 (char + 1 pad + int + short)
  if (sizeof(struct Packed2) != 8) abort();

  // __attribute__((packed)): sizeof = 7 (no padding)
  if (sizeof(struct AttrPacked) != 7) abort();

  return 0;
}
