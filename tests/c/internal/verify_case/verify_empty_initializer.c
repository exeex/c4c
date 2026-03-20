// Verify mode: empty initializer produces a diagnostic.

void f(void) {
    int x = ; // expected-error {{unexpected token in expression}}
}
