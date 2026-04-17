// Bootstrap EASTL external case for frontend-only coverage.
// This intentionally stays tiny so the suite wiring can be proven before we
// expand into broader runtime-oriented EASTL coverage.

#include <EASTL/internal/piecewise_construct_t.h>

int main() {
    eastl::piecewise_construct_t tag = eastl::piecewise_construct;
    (void)tag;
    return 0;
}
