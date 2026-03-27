// Parse-only regression: record post-body completion should preserve trailing
// attributes, nested record members, typedef members, and specialization
// self-type registration while `parse_struct_or_union()` lifts that handoff
// into a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct CompletionHandoff;

template <>
struct CompletionHandoff<int> {
    struct Inner {
        using Self = Inner;

        Self echo(Self other) { return other; }
    };

    using Self = CompletionHandoff;
    typedef int Count;

    Self bounce(Self other) { return other; }
    Count first, second;
    Inner payload;
} __attribute__((aligned(16)));

int main() {
    return 0;
}
