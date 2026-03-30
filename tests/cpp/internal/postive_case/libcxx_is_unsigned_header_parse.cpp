// Parse-only regression: libc++ should stay on the builtin branch of
// `__type_traits/is_unsigned.h` so the header parses without entering the
// fallback trait implementation path.
// RUN: %c4cll --parse-only %s

#include <__type_traits/is_unsigned.h>

int main() {
    return 0;
}
