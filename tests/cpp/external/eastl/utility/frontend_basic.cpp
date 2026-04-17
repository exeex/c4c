// Reduced EASTL utility smoke test derived from the upstream utility coverage.
// Keep this frontend-only so the external suite can grow case-by-case without
// pulling in the full EASTLTest runtime harness.

#include <EASTL/utility.h>

int main() {
    int left = 4;
    int right = 9;
    eastl::swap(left, right);
    return (left == 9 && right == 4) ? 0 : 1;
}
