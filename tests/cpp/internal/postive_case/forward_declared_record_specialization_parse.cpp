// Parse-only regression: explicit record specializations can be forward
// declarations, not only definitions with a body or base clause.
// RUN: %c4cll --parse-only %s

template <typename T>
class Box;

template <>
class Box<void>;

int main() {
    return 0;
}
