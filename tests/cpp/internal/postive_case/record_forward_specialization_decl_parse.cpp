// Parse-only regression: forward-declared explicit specializations should keep
// specialization args and record metadata intact while the record-definition
// builder handles the no-body path.
// RUN: %c4cll --parse-only %s

template <typename T, int N>
struct ForwardDeclCarrier;

template <>
struct __attribute__((packed)) ForwardDeclCarrier<int, 8>;

template <>
struct __attribute__((packed, aligned(16))) ForwardDeclCarrier<int, 8> {
    int payload;
};

int main() {
    return 0;
}
