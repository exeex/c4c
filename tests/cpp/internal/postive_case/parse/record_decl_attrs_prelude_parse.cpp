// Parse-only regression: record pre-body declaration attributes and alignas
// handling should stay stable while `parse_record_prebody_setup()` lifts that
// repeated prelude into a dedicated helper.
// RUN: %c4cll --parse-only %s

struct BaseForAttrPrelude {
    int payload;
};

template <typename T>
struct [[maybe_unused]] alignas(16) AttrPrelude [[maybe_unused]]
    : BaseForAttrPrelude {
    T value;
} __attribute__((aligned(32)));

int main() {
    return 0;
}
