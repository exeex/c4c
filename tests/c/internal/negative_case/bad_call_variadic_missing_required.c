// Expected: compile fail (variadic function called with fewer than required params)
int logf_like(const char *fmt, ...);

int main(void) {
  return logf_like();
}
