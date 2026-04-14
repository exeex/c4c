// Parse-only regression: reserve access labels and `virtual` as keyword tokens
// without breaking the existing record-body and base-clause compatibility
// paths that still accept those spellings.
// RUN: %c4cll --parse-only %s

struct LeftBase {};
struct RightBase {};

struct keyword_access_virtual_box : public virtual LeftBase, protected RightBase {
public:
    int x;

private:
    int tail;
};

int main() {
    keyword_access_virtual_box box;
    box.x = 1;
    return box.x - 1;
}
