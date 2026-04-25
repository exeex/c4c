// Parse-only regression: record-member template prelude typedef seeding plus
// record/enum self-tag registration should remain stable while
// `impl/types/struct.cpp` routes those mutations through parser-local
// helpers.
// RUN: %c4cll --parse-only %s

template <typename T>
struct PreludeAndTagRegistration;

template <>
struct PreludeAndTagRegistration<int> {
    template <typename U = PreludeAndTagRegistration>
    struct Holder {
        U* self;

        U* echo(U* other) { return other; }
    };

    using Self = PreludeAndTagRegistration;
    enum class Kind : unsigned int { Alpha, Beta };

    Self bounce(Self other) { return other; }
    Holder<> nested;
    Kind kind;
};

namespace enum_tag_registration {

enum class Color : unsigned int {
    Red,
    Blue
};

Color identity(Color color) { return color; }

struct Wrapper {
    using Self = Wrapper;

    Self echo(Self other) { return other; }
    Color color;
};

}  // namespace enum_tag_registration

int main() {
    return 0;
}
