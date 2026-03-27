// Parse-only regression: record body-context teardown should restore the
// surrounding self-type state after the closing brace so nested self-type
// parsing in one record does not leak into the next record while
// `parse_record_body_with_context()` delegates teardown to a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct ContextTeardown;

template <>
struct ContextTeardown<int> {
    struct Inner {
        using Self = Inner;

        Self echo(Self other) { return other; }
    };

    using Self = ContextTeardown;

    Self bounce(Self other) { return other; }
    Inner child;
};

struct AfterContextTeardown {
    using Self = AfterContextTeardown;

    Self echo(Self other) { return other; }
    ContextTeardown<int>::Inner nested;
};

int main() {
    return 0;
}
