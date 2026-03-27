// Parse-only: parenthesized member-pointer declarators should keep attribute
// skipping plus cv/nullability qualifier consumption stable while the prefix
// setup is extracted into a helper.
// RUN: %c4cll --parse-only %s

template <typename T>
struct Box {
    int value;
    int read() const { return value; }
};

template <typename T>
void accept(int (__attribute__((unused)) Box<T>::*_Nonnull const reader)() const) {
    (void)reader;
}

int main() {
    int (__attribute__((unused)) Box<int>::*_Nonnull const reader)() const =
        &Box<int>::read;
    accept<int>(reader);
    return 0;
}
