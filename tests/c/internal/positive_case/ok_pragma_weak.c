// Test #pragma weak on functions and globals
extern void abort(void);

#pragma weak weak_func
int weak_func(void) {
  return 42;
}

#pragma weak weak_var
int weak_var = 100;

int strong_func(void) {
  return 7;
}

int main(void) {
  // weak_func should still be callable
  if (weak_func() != 42) abort();

  // weak_var should still be accessible
  if (weak_var != 100) abort();

  // strong_func is normal linkage
  if (strong_func() != 7) abort();

  return 0;
}
