// Parse-only regression: the EASTL function_detail-style partial specialization
// should keep the heap-allocation CreateFunctor body in expression mode.
// RUN: %c4cll --parse-only %s

namespace eastl {

template <bool Cond, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <typename T>
T&& forward(T& value) {
    return static_cast<T&&>(value);
}

template <typename T>
T&& move(T& value) {
    return static_cast<T&&>(value);
}

struct allocator {
    void* allocate(unsigned long size, unsigned long align, int flags) {
        return size ? this : (align ? this : (flags ? this : this));
    }

    void deallocate(void* ptr, unsigned long size) {
        (void)ptr;
        (void)size;
    }
};

allocator* EASTLAllocatorDefault();

template <typename Functor, int Size>
struct is_functor_inplace_allocatable {
    static constexpr bool value = false;
};

namespace internal {
enum ManagerOperations {
    MGROPS_DESTRUCT_FUNCTOR,
    MGROPS_COPY_FUNCTOR,
    MGROPS_MOVE_FUNCTOR
};
}

template <int SizeInBytes, typename Enable = void>
class function_manager_base;

template <int SizeInBytes, typename Functor>
class function_manager_base<
    SizeInBytes,
    typename eastl::enable_if<
        !is_functor_inplace_allocatable<Functor, SizeInBytes>::value>::type> {
public:
    static Functor* GetFunctorPtr(void* storage) {
        return *static_cast<Functor**>(storage);
    }

    static Functor*& GetFunctorPtrRef(void* storage) {
        return *static_cast<Functor**>(storage);
    }

    template <typename T>
    static void CreateFunctor(void* storage, T&& functor) {
        auto& allocator = *EASTLAllocatorDefault();
        Functor* func = static_cast<Functor*>(
            allocator.allocate(sizeof(Functor), alignof(Functor), 0));
        ::new (static_cast<void*>(func)) Functor(eastl::forward<T>(functor));
        GetFunctorPtrRef(storage) = func;
    }

    static void DestructFunctor(void* storage) {
        Functor* func = GetFunctorPtr(storage);
        if (func) {
            auto& allocator = *EASTLAllocatorDefault();
            func->~Functor();
            allocator.deallocate(static_cast<void*>(func), sizeof(Functor));
        }
    }

    static void CopyFunctor(void* to, void* from) {
        auto& allocator = *EASTLAllocatorDefault();
        Functor* func = static_cast<Functor*>(
            allocator.allocate(sizeof(Functor), alignof(Functor), 0));
        ::new (static_cast<void*>(func)) Functor(*GetFunctorPtr(from));
        GetFunctorPtrRef(to) = func;
    }

    static void MoveFunctor(void* to, void* from) {
        auto& allocator = *EASTLAllocatorDefault();
        Functor* func = static_cast<Functor*>(
            allocator.allocate(sizeof(Functor), alignof(Functor), 0));
        ::new (static_cast<void*>(func))
            Functor(eastl::move(*GetFunctorPtr(from)));
        GetFunctorPtrRef(to) = func;
    }

    static void* Manager(void* to, void* from,
                         typename internal::ManagerOperations ops) {
        switch (ops) {
        case internal::MGROPS_DESTRUCT_FUNCTOR: {
            DestructFunctor(to);
        } break;
        case internal::MGROPS_COPY_FUNCTOR: {
            CopyFunctor(to, from);
        } break;
        case internal::MGROPS_MOVE_FUNCTOR: {
            MoveFunctor(to, from);
            DestructFunctor(from);
        } break;
        default:
            break;
        }
        return nullptr;
    }
};

}  // namespace eastl

struct Payload {
    Payload() = default;
    Payload(const Payload&) = default;
    explicit Payload(int) {}
};

int main() { return 0; }
