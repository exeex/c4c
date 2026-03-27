// Parse-only regression: nested struct/union members inside a record body
// should preserve declarator handling and anonymous-field synthesis.
// RUN: %c4cll --parse-only %s

struct RecordBody {
    struct Named {
        int x;
    } first, second;

    union Choice {
        int i;
        long l;
    } choice;

    struct {
        int hidden;
        union {
            int inner;
            long alt;
        };
    } anon;

    union {
        int direct;
        long fallback;
    };

    int tail;
};

int main() {
    return 0;
}
