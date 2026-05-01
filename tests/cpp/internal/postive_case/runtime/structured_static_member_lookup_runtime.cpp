namespace left {
struct Box {
    static constexpr int value = 11;
};
}

namespace right {
struct Box {
    static constexpr int value = 23;
};
}

int main() {
    if (left::Box::value != 11)
        return 1;
    if (right::Box::value != 23)
        return 2;
    return 0;
}
