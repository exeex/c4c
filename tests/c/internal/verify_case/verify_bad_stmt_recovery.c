// Verify mode: statement-level error recovery produces expected diagnostics.
// The compiler should report errors for bad statements and continue parsing.

int test_recovery(void) {
    int a = 1;

    @@@; // expected-error {{unexpected token in expression}}

    int b = 2;

    @@@; // expected-error {{unexpected token in expression}}

    int c = 3;
    return a + b + c;
}
