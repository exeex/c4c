// Frontend regression: using-declarations inside a namespace should import
// both typedef-backed types and value names through the namespace-aware parser
// lookup helpers.
namespace lib {
typedef int value_type;
int seed = 7;
}

namespace app {
using lib::value_type;
using lib::seed;

value_type read_seed() {
    value_type local = seed;
    return local;
}
}  // namespace app

int main() {
    return app::read_seed() == 7 ? 0 : 1;
}
