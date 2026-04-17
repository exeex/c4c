// Reduced from libstdc++ iterator_concepts.h: constrained type template
// parameters can use a qualified concept-id as the introducer.
// RUN: %c4cll --parse-only %s

namespace detail {
template<typename T>
concept Always = true;
}

template<detail::Always T>
struct boxed {
    T value;
};

int main() {
    return 0;
}
