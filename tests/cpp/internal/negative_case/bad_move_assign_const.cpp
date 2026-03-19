// Phase 3 negative: move assignment to const object should be rejected.
struct Obj {
    int val;
    Obj& operator=(Obj&& other) {
        val = other.val;
        return *this;
    }
};

int main() {
    const Obj a{10};
    Obj b{20};
    a = static_cast<Obj&&>(b);
    return 0;
}
