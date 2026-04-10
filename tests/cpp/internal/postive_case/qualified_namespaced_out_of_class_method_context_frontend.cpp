// Frontend regression: a fully qualified out-of-class method definition should
// keep the canonical owner record even when another namespace reuses the same
// unqualified record name.
namespace alpha {
struct allocator {
    int value;

    allocator& assign(const allocator& other);
    int read() const;
};
}

namespace beta {
struct allocator {
    int value;
};
}

alpha::allocator& alpha::allocator::assign(const alpha::allocator& other) {
    value = other.value;
    return *this;
}

int alpha::allocator::read() const {
    return value;
}

int main() {
    alpha::allocator lhs{1};
    alpha::allocator rhs{7};
    lhs.assign(rhs);
    return lhs.read() == 7 ? 0 : 1;
}
