// Parse-only regression: a forward-declared record should still be treated as
// complete for local declarations inside its own inline member bodies once the
// full definition is being parsed.
// RUN: %c4cll --parse-only %s

namespace std {
namespace ranges {
namespace __detail {

class A;

class A {
public:
    A();

    A& operator*=(A other) {
        A snapshot;
        (void)snapshot;
        return *this;
    }
};

}  // namespace __detail
}  // namespace ranges
}  // namespace std

int main() {
    return 0;
}
