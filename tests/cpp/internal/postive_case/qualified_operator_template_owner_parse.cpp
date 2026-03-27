// Parse-only: out-of-class operator declarators should accept template-id
// owners through the shared qualified declarator-name path.
// RUN: %c4cll --parse-only %s

template <typename T>
struct Box {
    int operator[](int index) const;
};

template <typename T>
int Box<T>::operator[](int index) const {
    return index;
}

int main() {
    Box<int> box;
    return box[0];
}
