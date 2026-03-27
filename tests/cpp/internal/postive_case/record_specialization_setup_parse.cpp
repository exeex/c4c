// Parse-only regression: template specialization record bodies should keep
// their source-name/self-type setup intact while `parse_struct_or_union()`
// lifts that setup into a focused helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct RecordSetup;

template <>
struct RecordSetup<int> {
    using Self = RecordSetup;

    RecordSetup() = default;
    RecordSetup(const RecordSetup&) = default;
    ~RecordSetup() = default;

    Self echo(Self other) { return other; }
    RecordSetup* next;
    Self shadow;
};

int main() {
    return 0;
}
