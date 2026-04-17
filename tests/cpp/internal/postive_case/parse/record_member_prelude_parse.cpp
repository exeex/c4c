// Parse-only regression: record-body preludes such as access labels, friend
// members, static_assert, and member-template setup should not interfere with
// the following declarations while the struct-body loop is flattened.
// RUN: %c4cll --parse-only %s

struct RecordPrelude {
public:
    template <typename U = int>
    struct Holder {
        U value;
    };

    friend bool operator==(const RecordPrelude&, const RecordPrelude&) {
        return true;
    }

    static_assert(sizeof(int) >= 4, "int width");

    template <typename U>
    U echo(U value) {
        return value;
    }

private:
    int tail;
};

int main() {
    return 0;
}
