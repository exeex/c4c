// Parse-only regression: record-body duplicate-field tracking should stay
// tolerant in C++ mode across malformed-member recovery while
// `parse_record_body()` lifts its duplicate-checker setup into a helper.
// RUN: %c4cll --parse-only %s

class RecordBodyDuplicateChecker {
public:
    using Alias = int;

    Alias value;
    int broken( { return 0; };
    Alias value;

    int total() const { return value + value; }
};

int main() {
    return 0;
}
