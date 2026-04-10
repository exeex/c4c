// Parse-only regression: declaration parsing should rebuild alias-template
// metadata and resolve qualified typedef bases through parser-local typedef
// lookup helpers instead of direct typedef table probes.
// RUN: %c4cll --parse-only %s

namespace api {

template <typename T>
struct box {
    typedef T value_type;
};

} // namespace api

template <typename T>
using alias_box = api::box<T>;

typedef api::box<int>::value_type qualified_value_t;
typedef alias_box<int>::value_type alias_template_value_t;

struct holder {
    alias_box<int> boxed;
    qualified_value_t first;
    alias_template_value_t second;
};

int main() {
    holder h{{}, 1, 2};
    return h.first + h.second - 3;
}
