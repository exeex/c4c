// Test #pragma GCC visibility push/pop on functions and globals
extern void abort(void);

#pragma GCC visibility push(hidden)

int hidden_func(void) {
  return 42;
}

int hidden_var = 100;

#pragma GCC visibility pop

int default_func(void) {
  return 7;
}

int default_var = 200;

#pragma GCC visibility push(protected)

int protected_func(void) {
  return 13;
}

#pragma GCC visibility pop

int main(void) {
  if (hidden_func() != 42) abort();
  if (hidden_var != 100) abort();
  if (default_func() != 7) abort();
  if (default_var != 200) abort();
  if (protected_func() != 13) abort();
  return 0;
}
