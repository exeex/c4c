// Parse-only regression: record pre-body base-clause parsing should preserve
// access/virtual qualifiers and template-id bases while
// `parse_record_prebody_setup()` lifts that path into focused helpers.
// RUN: %c4cll --parse-only %s

template <typename T>
struct LeftBase {
    T left;
};

template <typename T>
struct RightBase {
    T right;
};

template <typename T>
struct BaseClauseSetup;

template <>
struct BaseClauseSetup<int> : public virtual LeftBase<int>, protected RightBase<int> {
    using Self = BaseClauseSetup;

    Self echo(Self other) { return other; }
    int value;
};

int main() {
    return 0;
}
