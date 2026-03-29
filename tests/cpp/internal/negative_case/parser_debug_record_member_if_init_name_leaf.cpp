// Parser-debug regression: keep the committed expression leaf inside a
// record-member method body when an `if (size_type n = ...)` statement falls
// through the expression parser instead of reporting a wrapper summary.

using size_t = unsigned long;

class Vec {
    using size_type = size_t;
    int* finish;

public:
    void erase_at_end(int* pos) {
        if (size_type n = finish - pos)
            (void)n;
    }
};
