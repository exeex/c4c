static volatile int failures;
static volatile int observed_tags;

static void __attribute__((noinline)) check(int lhs_rank,
                                            int rhs_rank,
                                            int tag) {
  observed_tags += tag;
  failures += lhs_rank != rhs_rank;
}

#define PROMOTED_CALL(X, T)                                                    \
  check((int)(X), (int)(X), (int)sizeof((T)1))

#define CALL_BLOCK(X)                                                          \
  PROMOTED_CALL((signed char)(X), short);                                      \
  PROMOTED_CALL((unsigned char)(X), short);                                    \
  PROMOTED_CALL((signed char)(X), unsigned short);                             \
  PROMOTED_CALL((unsigned char)(X), unsigned short);                           \
  PROMOTED_CALL((short)(X), int);                                              \
  PROMOTED_CALL((unsigned short)(X), int);                                     \
  PROMOTED_CALL((short)(X), unsigned int);                                     \
  PROMOTED_CALL((unsigned short)(X), unsigned int)

#define CALL_GROUP(X)                                                          \
  CALL_BLOCK(X);                                                               \
  CALL_BLOCK(-(X));                                                            \
  CALL_BLOCK((X) + 1);                                                         \
  CALL_BLOCK(-((X) + 1))

int main(int argc, char **argv) {
  observed_tags += argc + (argv != 0);
  CALL_GROUP(1);
  CALL_GROUP(2);
  CALL_GROUP(3);
  CALL_GROUP(4);
  CALL_GROUP(5);
  CALL_GROUP(6);
  CALL_GROUP(7);
  CALL_GROUP(8);
  CALL_GROUP(9);
  CALL_GROUP(10);
  CALL_GROUP(11);
  CALL_GROUP(12);
  CALL_GROUP(13);
  CALL_GROUP(14);
  CALL_GROUP(15);
  CALL_GROUP(16);
  CALL_GROUP(17);
  CALL_GROUP(18);
  CALL_GROUP(19);
  CALL_GROUP(20);
  CALL_GROUP(21);
  CALL_GROUP(22);
  CALL_GROUP(23);
  CALL_GROUP(24);
  CALL_GROUP(25);
  CALL_GROUP(26);
  CALL_GROUP(27);
  CALL_GROUP(28);
  CALL_GROUP(29);
  CALL_GROUP(30);
  CALL_GROUP(31);
  CALL_GROUP(32);
  return failures != 0;
}
