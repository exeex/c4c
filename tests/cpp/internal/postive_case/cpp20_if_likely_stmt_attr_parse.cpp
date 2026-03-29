// Reduced from libstdc++ bits/max_size_type.h: statement attributes can appear
// between an if-condition and the controlled statement.
// RUN: %c4cll --parse-only %s

int main() {
    int a = 8;
    int b = 2;
    if (a && b) [[likely]]
        a /= b;
    else
        a %= b;
    return a;
}
