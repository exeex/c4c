// Verify mode: function call arity mismatch.

int add(int a, int b) { return a + b; }

int main(void) {
    return add(1); // expected-error {{function call arity mismatch}}
}
