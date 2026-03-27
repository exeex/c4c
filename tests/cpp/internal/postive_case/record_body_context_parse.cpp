// Parse-only regression: record body-context setup and restore should keep
// nested record parsing from clobbering the outer self-type context while
// `parse_struct_or_union()` lifts that lifecycle into a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct BodyContext;

template <>
struct BodyContext<int> {
    struct Inner {
        using Self = Inner;

        Self echo(Self other) { return other; }
    };

    using Self = BodyContext;

    BodyContext() = default;
    BodyContext(const BodyContext&) = default;
    ~BodyContext() = default;

    Self bounce(Self other) { return other; }
    Inner child;
};

int main() {
    return 0;
}
