// Reduced preprocessor branch-selection test for libstdc++-style C++20 gates.
// RUN: %c4cll --parse-only %s

#if __cplusplus > 201703L && __cpp_concepts >= 201907L && __cpp_constexpr >= 201811L
template<typename T>
struct selected_branch {
    using pointer = T;
};
#else
template<typename T>
struct selected_branch {
    typedef missing_type pointer;
};
#endif

int main() {
    selected_branch<int> x;
    (void)x;
    return 0;
}
