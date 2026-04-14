// Parse-only regression: declaration-context qualified type probing should
// share unresolved global-qualified fallback classification with
// `parse_base_type()` without misclassifying neighboring qualified calls.
// RUN: %c4cll --parse-only %s

namespace api {

struct widget {
    int value;
};

int make_value() {
    return 11;
}

} // namespace api

void test() {
    ::api::widget item{};
    int value = ::api::make_value();
    item.value = value;
}

int main() {
    test();
    return 0;
}
