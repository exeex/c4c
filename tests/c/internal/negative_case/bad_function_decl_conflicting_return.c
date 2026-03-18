// Expected: compile fail (conflicting function declarations: return type mismatch)
int foo(void);
double foo(void);

int main(void) {
  return 0;
}
