// Parse-only regression: record tag setup should preserve namespace-qualified
// forward references, same-name definitions in distinct namespaces, and
// shadow-tag handling for repeated definitions while `parse_struct_or_union()`
// lifts the remaining tag/setup branch into a helper.
// RUN: %c4cll --parse-only %s

namespace alpha {
struct Shared;
struct Shared {
    int value;
};
}

namespace beta {
struct Shared;
struct Shared {
    int left;
    int right;
};
}

template <typename T>
struct Holder;

template <>
struct Holder<alpha::Shared> {
    alpha::Shared value;
};

template <>
struct Holder<beta::Shared> {
    beta::Shared value;
};

struct ShadowCarrier {
    struct Duplicate {
        int first;
    };

    struct Duplicate {
        int second;
    };

    Duplicate payload;
};

int main() {
    return 0;
}
