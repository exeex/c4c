// Parse-only regression: record body collection should preserve nested record,
// typedef/using members, methods, and fields while `parse_struct_or_union()`
// collapses its local body accumulators into a focused helper-owned bundle.
// RUN: %c4cll --parse-only %s

template <typename T>
struct BodyStateBundle;

template <>
struct BodyStateBundle<int> {
    struct Inner {
        using Value = int;
        Value payload;
    };

    typedef int Count;
    using Alias = Inner;

    Count first, second;
    Alias child;

    Count total() { return first + second; }
};

int main() {
    return 0;
}
