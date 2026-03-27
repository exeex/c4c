template<typename T>
T countdown(T n) {
    if (n <= 0) return n;
    return countdown<T>(n - 1);
}

int main() {
    return countdown<int>(3);
}
