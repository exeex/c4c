// Expected: compile fail (sizeof on incomplete object expression)
struct S;

int main(void) {
  struct S *p = 0;
  return sizeof(*p);
}
