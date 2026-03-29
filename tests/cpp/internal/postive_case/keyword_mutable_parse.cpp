// Parse-only regression: reserve `mutable` as a keyword token without breaking
// record-member declarations or lambda-specifier parsing that previously
// tolerated the raw identifier spelling.
// RUN: %c4cll --parse-only %s

struct keyword_mutable_box {
    mutable int cache;

    int bump() const {
        auto touch = [value = cache]() mutable -> int {
            value += 1;
            return value;
        };
        cache = touch();
        return cache;
    }
};

int main() {
    keyword_mutable_box box{4};
    return box.bump() - 5;
}
