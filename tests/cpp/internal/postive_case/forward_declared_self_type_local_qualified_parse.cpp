// Parse-only regression: self-type local declarations should also work when
// spelled with a fully-qualified name after a forward declaration.
// RUN: %c4cll --parse-only %s

namespace std {
namespace ranges {
namespace __detail {

class A;

class A {
public:
    A();

    A& operator*=(A other) {
        std::ranges::__detail::A mirror;
        (void)mirror;
        return *this;
    }
};

}  // namespace __detail
}  // namespace ranges
}  // namespace std

int main() {
    return 0;
}
