consteval bool foo(int x) {
    return x > 5;
}

static_assert(foo(6), "consteval positive static_assert should pass");

int main() {
    return foo(6) ? 0 : 1;
}
