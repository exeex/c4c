struct T {
    ~T();
};

int main() {
    T t;
    int x = 0;
    if (x == 0)
        t.~T();
    else if (x == 1)
        t.~T();
    return 0;
}
