// Parse-only test for EASTL bring-up slice 4:
// - typename dependent type consumption (typename X::Y::Z)
// - template specialization typedef/struct redefinition tolerance
// - unresolved qualified names in C++ mode (ns::Type)
// - constexpr/consteval in parse_base_type
// - static constexpr data member initializer skipping
// - friend declarations in struct bodies
// - access specifiers (public/private/protected) in struct bodies
// RUN: %c4cll --parse-only %s

namespace eastl {

// --- typename dependent type consumption ---
template<typename T> struct iterator_traits {
    typedef T value_type;
    typedef int difference_type;
};

template<typename Iterator>
struct move_iterator {
    typedef typename eastl::iterator_traits<Iterator>::difference_type difference_type;
    typedef typename eastl::iterator_traits<Iterator>::value_type value_type;
    move_iterator operator+(difference_type n) const { return *this; }
};

// --- template specialization with same inner typedef/struct ---
template <typename T, bool B>
struct make_signed_helper {
    struct no_type_helper {};
    typedef no_type_helper type;
};

template <typename T>
struct make_signed_helper<T, false> {
    struct no_type_helper {};
    typedef no_type_helper type;
};

// --- constexpr data member with initializer ---
template<typename T> struct is_void { static constexpr bool value = false; };
template<> struct is_void<void> { static constexpr bool value = true; };

template<typename T> struct is_same_as_int { static constexpr bool value = false; };
template<> struct is_same_as_int<int> { static constexpr bool value = true; };

// --- access specifiers and friend ---
template<typename T>
struct container {
public:
    typedef T value_type;
private:
    T* data_;
    int size_;
protected:
    int capacity_;
public:
    int size() const { return size_; }
    friend iterator_traits<T>;
};

// --- unresolved qualified name in template specialization args ---
struct input_tag {};
struct random_access_tag {};

template<typename Tag, bool A, bool B>
struct copy_helper {};

template<>
struct copy_helper<eastl::random_access_tag, false, false> {};

// --- unresolved qualified name in function parameters ---
template <typename Iterator>
int distance_impl(Iterator first, Iterator last, eastl::input_tag) {
    return 0;
}

// --- typename with nested template and ::type suffix ---
template<bool B, typename T> struct enable_if {};
template<typename T> struct enable_if<true, T> { typedef T type; };

template<typename T>
typename eastl::enable_if<true, T>::type identity(T val) {
    return val;
}

} // namespace eastl

int main() {
    // Validate that types are correctly parsed
    eastl::container<int> c;
    eastl::move_iterator<int*> mi;

    // Template specialization inner type access
    eastl::make_signed_helper<int, true>::type t1;
    eastl::make_signed_helper<int, false>::type t2;

    return 0;
}
