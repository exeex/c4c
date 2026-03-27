// Parse-only: parenthesized pointer-to-member-function declarators should
// accept owner spellings that include template arguments before `::*`.
// RUN: %c4cll --parse-only %s

template <typename T>
struct Box {
    int get() const { return 0; }
};

template <typename T>
void accept(int (Box<T>::*pm)() const) {
    (void)pm;
}

int main() {
    int (Box<int>::*pm)() const = &Box<int>::get;
    accept<int>(pm);
    return 0;
}
