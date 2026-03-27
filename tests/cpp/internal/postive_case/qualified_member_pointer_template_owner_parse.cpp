// Parse-only: pointer-to-member declarators should accept owner spellings
// that include template arguments before the scope operator.
// RUN: %c4cll --parse-only %s

template <typename T>
struct Box {
    int value;
};

template <typename T>
void accept(int Box<T>::*member) {
    (void)member;
}

int main() {
    int Box<int>::*member = 0;
    accept<int>(member);
    return 0;
}
