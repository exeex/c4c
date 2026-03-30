// Parse-only regression: scoped enums with an explicit underlying type should
// remain enum definitions instead of falling into record parsing.
// RUN: %c4cll --parse-only %s

namespace demo {
enum class byte : unsigned char {};
}

demo::byte identity(demo::byte value) {
    return value;
}

int main() {
    return 0;
}
