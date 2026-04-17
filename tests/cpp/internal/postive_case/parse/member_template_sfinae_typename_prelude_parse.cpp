// Parse-only regression: member-template preludes should accept dependent
// `typename ...::type` heads for both defaulted type parameters and typed NTTPs
// without needing a dedicated parser fork from the shared template-parameter
// paths.
// RUN: %c4cll --parse-only %s

template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename T>
struct holder {
    using value_type = T;
};

struct MemberTemplateSfinaeTypenamePreludeProbe {
    template <typename T, typename = typename holder<T>::value_type>
    static int defaulted_type();

    template <typename T,
              typename enable_if<true, typename holder<T>::value_type>::type = 0>
    static int unnamed_typed_nttp();

    template <typename T,
              typename enable_if<true, typename holder<T>::value_type>::type Dummy = 0>
    struct named_typed_nttp {
        static constexpr int value = Dummy;
    };
};

int main() {
    return 0;
}
