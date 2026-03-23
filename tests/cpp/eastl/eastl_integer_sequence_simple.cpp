#include <EASTL/internal/integer_sequence.h>

int main() {
    using seq = eastl::make_index_sequence<4>;
    return eastl::internal::index_sequence_size_v<seq> == 4 ? 0 : 1;
}
