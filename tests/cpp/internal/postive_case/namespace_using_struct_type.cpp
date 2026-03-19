// Test: 'using namespace' makes struct types from that namespace visible.
namespace lib {
struct Data {
    int a;
    int b;
};
}

using namespace lib;

int main() {
    Data d;
    d.a = 5;
    d.b = 10;
    if (d.a + d.b != 15) return 1;
    return 0;
}
