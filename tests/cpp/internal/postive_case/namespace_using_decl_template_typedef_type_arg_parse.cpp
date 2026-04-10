// Parse-only regression: namespace-imported typedefs should stay visible to
// the shared type-parser lookup helpers used by declaration and template-arg
// probes.
// RUN: %c4cll --parse-only %s

namespace lib {

template <typename T>
struct box {
    T value;
};

typedef int value_type;

}  // namespace lib

namespace app {

using lib::value_type;

lib::box<value_type> make_box(value_type input) {
    lib::box<value_type> result{};
    result.value = input;
    return result;
}

}  // namespace app

int main() {
    app::make_box(3);
    return 0;
}
