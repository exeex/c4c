// Parse-only regression: top-level function return types using
// `decltype(Impl::unwrap(std::declval<T>()))` should fall back to the shared
// balanced decltype skip path instead of committing to a type-name probe.
// RUN: %c4cll --parse-only %s

namespace std {
template <class T>
T&& declval();
} // namespace std

struct Impl {
    static int unwrap(int);
};

inline decltype(Impl::unwrap(std::declval<int>())) probe(int i) {
    return Impl::unwrap(i);
}

int main() {
    return probe(0);
}
