// Parse-only regression: record definition setup should preserve template
// specialization args, multi-base transfer, and packed/aligned layout state
// while `parse_struct_or_union()` lifts node initialization into a helper.
// RUN: %c4cll --parse-only %s

struct LeftBase {
    int left;
};

struct RightBase {
    int right;
};

template <typename T, int N>
struct SetupCarrier;

template <>
struct __attribute__((packed)) SetupCarrier<int, 4> : LeftBase, RightBase {
    using Self = SetupCarrier;

    Self echo(Self other) { return other; }
    int payload;
} __attribute__((aligned(32)));

int main() {
    return 0;
}
