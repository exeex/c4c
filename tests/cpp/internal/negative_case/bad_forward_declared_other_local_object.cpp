// Negative test: only the current self-type should bypass forward-declared
// incompleteness inside an inline member body.
namespace std {
namespace ranges {
namespace __detail {

class Other;
class A;

class A {
public:
    A();

    A& operator*=(A other) {
        Other not_ready;
        (void)not_ready;
        return *this;
    }
};

}  // namespace __detail
}  // namespace ranges
}  // namespace std

int main() {
    return 0;
}
