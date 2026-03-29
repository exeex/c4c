// Parser-debug regression: keep the committed expression leaf inside a
// record-member method body when an `if (const size_t ...)` statement triggers
// a speculative qualified-type probe.

using size_t = unsigned long;

class Broken {
    static bool equal(const int* first, const int* last) {
        if (const size_t len = (last - first))
            return true;
        return false;
    }
};
