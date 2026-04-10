#include <EASTL/type_traits.h>

int main() {
    if (!eastl::is_same_v<eastl::remove_cv_t<const unsigned int>, unsigned int>)
        return 3;
    if (!eastl::is_same_v<eastl::remove_cv_t<volatile unsigned int>, unsigned int>)
        return 4;
    if (!eastl::is_same_v<eastl::remove_cv_t<const volatile unsigned int>, unsigned int>)
        return 5;
    return 0;
}
