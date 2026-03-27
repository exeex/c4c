// Parse-only regression: declaration-context qualified type probing should
// share classification with qualified base-type dispatch without
// misclassifying same-scope qualified function calls.
// RUN: %c4cll --parse-only %s

namespace api {

struct widget {
    int value;
};

int make_value() {
    return 7;
}

} // namespace api

void test() {
    api::widget item{};
    int value = api::make_value();
    item.value = value;
}

int main() {
    test();
    return 0;
}
