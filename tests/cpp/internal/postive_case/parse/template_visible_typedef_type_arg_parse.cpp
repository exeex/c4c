// Parse-only regression: template type parsing should resolve both qualified
// and using-imported typedef names through parser-local typedef lookup helpers.
// RUN: %c4cll --parse-only %s

namespace lib {

typedef int value_type;

template <typename T>
struct wrap {
    using type = T;
};

}  // namespace lib

namespace app {

using lib::value_type;

}  // namespace app

using lib::value_type;

template <typename T = lib::wrap<app::value_type>::type>
struct holder {
    T value;
};

holder<> first{};
holder<value_type> second{};

int main() {
    return first.value + second.value;
}
