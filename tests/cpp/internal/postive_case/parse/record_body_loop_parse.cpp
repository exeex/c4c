// Parse-only regression: the record-body loop should keep mixed member
// dispatch, malformed-member recovery, and duplicate-field handling stable
// while `parse_struct_or_union()` shrinks toward a setup/body/finalization
// coordinator.
// RUN: %c4cll --parse-only %s

class RecordBodyLoop {
public:
    template <typename T = int>
    struct Holder {
        T value;
    };

    using Alias = Holder<>;
    enum Kind { Zero, One } kind;

    int broken( { return 0; };

    RecordBodyLoop() = default;
    ~RecordBodyLoop() = default;

    int first, second;
    static constexpr int seed = 7;

    int total() const { return first + second + seed; }

private:
    Alias payload;
};

int main() {
    return 0;
}
