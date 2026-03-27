template<typename... Args>
int first_plus_last(Args... args) {
    return 0;
}

int main() {
    return first_plus_last<int, long>(1, 2L);
}
