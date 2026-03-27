// Parse-only regression: template specialization record bodies should keep
// nested record parsing and outer self-type member dispatch stable while the
// post-prelude record-member category chain is lifted into a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct RecordMemberSpecialization;

template <>
struct RecordMemberSpecialization<int> {
    struct Inner {
        using Self = Inner;

        Self echo(Self other) { return other; }
    };

    using Self = RecordMemberSpecialization;
    typedef int Count;
    enum Kind { Zero, One } kind;

    RecordMemberSpecialization() = default;
    ~RecordMemberSpecialization() = default;

    Self bounce(Self other) { return other; }
    Count value;
    Inner child;
};

int main() {
    return 0;
}
