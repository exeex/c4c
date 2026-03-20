// Expected: compile fail (multiple statement-level errors with recovery)
// Tests that multiple bad statements don't prevent subsequent parsing.

int test_multi_stmt_recovery(void) {
    int a = 1;
    @@@ badtoken1;
    int b = 2;
    @@@ badtoken2;
    int c = 3;
    return a + b + c;
}
