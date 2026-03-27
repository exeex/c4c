// Parse-only regression: member-template prelude setup and cleanup should stay
// local to the templated member while ordinary self-type members and later
// member templates still parse after `try_parse_record_member()` sheds that
// lifecycle into a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct RecordTemplateScopeCleanup;

template <>
struct RecordTemplateScopeCleanup<int> {
    template <typename U = int>
    struct Holder {
        U value;

        U echo(U other) { return other; }
    };

    using Self = RecordTemplateScopeCleanup;

    Self bounce(Self other) { return other; }
    Holder<> payload;

    template <typename U>
    U wrap(U other) { return other; }
};

int main() {
    return 0;
}
