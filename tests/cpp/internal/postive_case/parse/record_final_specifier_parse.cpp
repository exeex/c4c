// Reduced from libstdc++ iterator_concepts.h: record definitions can carry a
// contextual `final` specifier between the class name and body.
// RUN: %c4cll --parse-only %s

namespace ranges::__access {
struct _Decay_copy final {
    template<typename T>
    constexpr T operator()(T&& t) const {
        return static_cast<T&&>(t);
    }
};
}

int main() {
    return 0;
}
