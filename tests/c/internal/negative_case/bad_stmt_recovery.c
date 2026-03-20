// Expected: compile fail (statement-level error recovery)
// This test verifies that a bad statement inside a function body
// does not prevent later statements/functions from being parsed.

int good_before(void) { return 1; }

int test_recovery(int x) {
    int a = 10;
    // Error: bad statement (missing operator/semicolon gibberish)
    @@@;
    int b = 20;
    return a + b;
}

int good_after(void) { return 2; }
