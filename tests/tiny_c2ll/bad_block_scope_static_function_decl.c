// Expected: compile fail (block-scope static object has incomplete type)
struct LocalTag;

int main(void) {
  static struct LocalTag x;
  return 0;
}
