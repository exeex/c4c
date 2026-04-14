// EASTL slice 7d: qualified declarator template args, empty template args,
// function type in template args, variable template expressions, and
// C-style cast vs parenthesized expression disambiguation.
// Parse-only test (main returns 0).

// Pattern 1: out-of-class method definition with template args in class name
template <typename T, typename Allocator>
struct vector {
    typedef unsigned long size_type;
    T* mpBegin;
    T* mpEnd;
    void set_capacity(size_type n);
    void DoFree(T* p, size_type cap);
};

template <typename T, typename Allocator>
void vector<T, Allocator>::set_capacity(size_type n) {
    T* pNewData = mpBegin;
    DoFree(mpBegin, (size_type)(mpEnd - mpBegin));
    int x = 0;
    int y = x + 1;
}

// Pattern 2: empty template args <> followed by {}
namespace eastl {
    template <typename T = void>
    struct less {};
}

template <typename ForwardIterator, typename T>
void equal_range(ForwardIterator first, ForwardIterator last, const T& value) {
    auto cmp = eastl::less<>{};
    (void)cmp;
}

// Pattern 3: function type R(Args...) as template argument
template <typename Sig>
struct function {};

template <typename R, typename... Args>
bool operator==(const function<R(Args...)>& f, int x) {
    return false;
}

// Pattern 4: nested method call with multiple args after namespace call
namespace eastl2 {
    template <typename T>
    void destruct(T* a, T* b) {}
}

template <typename T, typename A>
struct vec2 {
    typedef unsigned long size_type;
    T* b;
    T* e;
    void cap(size_type n);
    void free2(T* p, size_type s);
};

template <typename T, typename A>
void vec2<T, A>::cap(size_type n) {
    eastl2::destruct(b, e);
    free2(b, (size_type)(e - b));
}

// Pattern 5: variable template in expression context (is_signed_v<T>)
namespace ns {
    template <typename T>
    constexpr bool is_signed_v = false;
}

template <typename T>
void test_var_template() {
    if ((ns::is_signed_v<T>))
        return;
}

// Pattern 6: parenthesized namespace-qualified value
namespace ns2 {
    bool flag = false;
}

void test_paren_ns_value() {
    bool x = (ns2::flag);
}

// Pattern 7: template instantiation followed by various operators
template <typename T>
struct traits {
    static const int value = 0;
};

template <typename T>
void test_template_ops() {
    int a = traits<T>::value;
    bool b = traits<T>::value == 0;
    bool c = traits<T>::value != 1;
}

int main() {
    return 0;
}
