// Parse-only: parenthesized member-function-pointer declarator suffixes should
// keep parameter parsing, trailing cv-qualifier consumption, and storage
// wiring stable during helper extraction.
// RUN: %c4cll --parse-only %s

template <typename T>
struct Box {
    int value;
    int read() const { return value; }
    int read_volatile() volatile { return value; }
};

template <typename T>
void accept(int (Box<T>::*reader)() const,
            int (Box<T>::*volatile_reader)() volatile) {
    (void)reader;
    (void)volatile_reader;
}

int main() {
    int (Box<int>::*reader)() const = &Box<int>::read;
    int (Box<int>::*volatile_reader)() volatile = &Box<int>::read_volatile;
    accept<int>(reader, volatile_reader);
    return 0;
}
