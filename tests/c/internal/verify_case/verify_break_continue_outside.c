// Verify mode: break/continue outside loop or switch.

void test_break(void) {
    break; // expected-error {{break statement not within loop or switch}}
}

void test_continue(void) {
    continue; // expected-error {{continue statement not within loop}}
}
