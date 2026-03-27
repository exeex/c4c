// Parse-only regression: record pre-body setup should preserve attribute
// capture, specialization-tag handling, and base-clause parsing while
// `parse_struct_or_union()` lifts the prelude into a focused helper.
// RUN: %c4cll --parse-only %s

struct AlignBase {
    int payload;
};

template <typename T>
struct PreludeSetup;

template <>
struct __attribute__((packed)) PreludeSetup<int> : AlignBase {
    using Self = PreludeSetup;

    Self echo(Self other) { return other; }
    int value;
} __attribute__((aligned(32)));

int main() {
    return 0;
}
