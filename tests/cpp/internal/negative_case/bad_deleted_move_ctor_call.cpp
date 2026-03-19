// Phase 3 negative: calling a deleted move constructor should be rejected.
struct Obj {
    int val;
    Obj(int v) { val = v; }
    Obj(Obj&&) = delete;
};

int main() {
    Obj a(10);
    Obj b(static_cast<Obj&&>(a));
    return b.val;
}
