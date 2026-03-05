// Expected: compile fail (typedef redefined to different type)
typedef int T;
typedef char T;

int main() {
  return 0;
}
