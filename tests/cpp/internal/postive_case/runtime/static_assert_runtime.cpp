static_assert(sizeof(int) >= 4, "int width");

struct Holder {
    static_assert(sizeof(long long) >= sizeof(int), "long long width");
    int value;
};

int main() {
    static_assert(true, "local static_assert should survive lowering");
    Holder h{7};
    return h.value - 7;
}
