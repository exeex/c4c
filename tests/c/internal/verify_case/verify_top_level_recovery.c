// Verify mode: top-level parse error with recovery.
// The compiler should report the error and continue parsing the next function.

int bad_fn(int a { return a; } // expected-error {{expected=RPAREN}}

int good_fn(void) { return 42; }
