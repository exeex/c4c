// Test: template struct in namespace, qualified instantiation.
namespace coll {
template <typename T>
struct Box {
    T val;
    T get() const { return val; }
};
}

int main() {
    coll::Box<int> b;
    b.val = 42;
    if (b.get() != 42) return 1;

    coll::Box<long> lb;
    lb.val = 99;
    if (lb.get() != 99) return 2;
    return 0;
}
