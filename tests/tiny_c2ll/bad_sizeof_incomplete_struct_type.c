// Expected: compile fail (sizeof on incomplete struct type)
struct S;

int main(void) {
  return sizeof(struct S);
}
