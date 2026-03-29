consteval bool foo(int x) {
    return x > 5;
}

static_assert(foo(4), "consteval negative static_assert should fail");

int main() {
    return 0;
}
