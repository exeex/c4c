// Expected: compile fail (object of incomplete struct type)
struct S;
struct S obj;

int main() {
  return 0;
}
