// Parse-only regression: statement-level declaration probes should use
// parser-local typedef/value helpers for local aliases, shadowed values, and
// qualified static member calls.
// RUN: %c4cll --parse-only %s

namespace api {

struct widget {
    static int make() { return 1; }
};

}  // namespace api

void test() {
    using Alias = api::widget;
    using Count = int;

    Alias::make();

    {
        int Count = 1;
        Count = Count + 1;
    }

    if (Count current = 1) {
        (void)current;
    }
}

int main() {
    test();
    return 0;
}
