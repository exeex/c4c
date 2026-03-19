// Expected: compile fail (multiple parse errors with recovery)
// This test verifies that the compiler reports errors in
// file:line:col: error: format and can recover past errors.

int good_before(void) { return 1; }

// Error 1: missing closing paren
int bad_fn1(int a, int b { return a; }

// A good function after error 1 to let parser resync
int good_middle(void) { return 42; }

// Error 2: another missing paren (independent of error 1)
int bad_fn2(int x { return x; }

int good_after(void) { return 2; }
