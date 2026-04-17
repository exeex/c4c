// Parse-only regression: the outer record-definition dispatcher should
// preserve specialization setup, nested-member parsing, and trailing
// attributes while `parse_struct_or_union()` delegates the post-tag-setup
// definition path to one focused helper.
// RUN: %c4cll --parse-only %s

struct DispatchBase {
    int base;
};

template <typename T, int N>
struct DispatchCarrier;

template <>
struct __attribute__((packed)) DispatchCarrier<int, 2> : DispatchBase {
    union Inner {
        int i;
        float f;
    };

    using Self = DispatchCarrier;

    Self echo(Self other) { return other; }
    Inner payload;
    int count;
} __attribute__((aligned(16)));

int main() {
    return 0;
}
