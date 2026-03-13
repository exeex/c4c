// Expected: compile fail (typedef refers to itself as a type)
typedef T T;

int main() {
  return 0;
}
