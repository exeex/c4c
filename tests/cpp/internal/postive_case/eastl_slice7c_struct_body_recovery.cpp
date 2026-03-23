// EASTL slice 7c: struct body error recovery, C++ casts, lambdas
// Tests that lambda expressions in template functions are skipped,
// and that template struct bodies with C++ casts parse correctly.
// This is a parse-only test (main returns 0).

// Lambda in template function (parsed but skipped as placeholder)
template <typename T>
T identity(T x) {
    auto fn = [](T v) -> T { return v; };
    (void)fn;
    return x;
}

// Nested template class with static_cast in method body
template <int SIZE>
struct FuncBase {
    template <typename Functor>
    struct Manager {
        static Functor* Create(void* storage) {
            return static_cast<Functor*>(storage);
        }
    };
    int data;
};

// Constructor/destructor detection for partial specialization
template <int, typename>
struct FuncDetail;

template <int N, typename R, typename A>
struct FuncDetail<N, R(A)> {
    FuncDetail() = default;
    FuncDetail(const FuncDetail&) = default;
    FuncDetail(FuncDetail&&) = default;
    ~FuncDetail() = default;
    FuncDetail& operator=(const FuncDetail&) = default;
    int value;
};

int main() {
    return 0;
}
