// Parse-only regression: `parse_base_type()` should keep dispatching
// qualified names through the same resolution order for resolved typedefs and
// unresolved qualified fallback types.
// RUN: %c4cll --parse-only %s

using global_size = unsigned long;

namespace api {

typedef int count_t;

struct widget {
    int value;
};

} // namespace api

void test() {
    ::global_size global_count = 0;
    api::count_t count = 1;
    api::widget item{};
    item.value = count;
    (void)global_count;
}

int main() {
    test();
    return 0;
}
