// Parse-only: declaration contexts should treat qualified typedef-like names
// as type starts through the shared qualified-name resolution path.
// RUN: %c4cll --parse-only %s

using size_t = unsigned long;

namespace ns {

struct Traits {
    typedef int value_type;
};

using count_t = unsigned long;

} // namespace ns

void test() {
    ::size_t global_count = 0;
    ns::count_t count = 1;
    ns::Traits::value_type value = 2;
    (void)global_count;
    (void)count;
    (void)value;
}

int main() {
    test();
    return 0;
}
