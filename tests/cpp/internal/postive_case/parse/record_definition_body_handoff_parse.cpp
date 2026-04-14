// Parse-only regression: the post-setup record-definition body handoff should
// preserve nested body parsing, member collection, and trailing attributes
// while `parse_struct_or_union()` delegates that path to one focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct BodyHandoff;

template <>
struct BodyHandoff<int> {
    struct Inner {
        using Self = Inner;

        Self bounce(Self other) { return other; }
    };

    using Self = BodyHandoff;
    typedef int Count;

    Self bounce(Self other) { return other; }
    Count first, second;
    Inner payload;
} __attribute__((packed, aligned(8)));

int main() {
    return 0;
}
