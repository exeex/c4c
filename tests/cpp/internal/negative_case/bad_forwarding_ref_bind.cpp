// Phase 4 negative: non-template T&& parameter rejects lvalue argument.
void consume(int&& x) {}

int main() {
    int a = 5;
    consume(a);
    return 0;
}
