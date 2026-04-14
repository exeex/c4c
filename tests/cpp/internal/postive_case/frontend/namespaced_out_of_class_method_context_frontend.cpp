// Frontend regression: namespaced out-of-class methods should keep the owning
// record context for implicit member lookup and `return *this;`.
namespace other {
struct allocator {};
}

namespace eastl {
struct allocator {
    int value;

    allocator& operator=(const allocator& other);
    int read() const;
};

allocator& allocator::operator=(const allocator& other) {
    value = other.value;
    return *this;
}

int allocator::read() const {
    return value;
}
}  // namespace eastl
