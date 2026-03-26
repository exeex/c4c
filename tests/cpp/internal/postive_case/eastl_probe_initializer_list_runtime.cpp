// Probe: initializer_list runtime behavior should only succeed when brace-init,
// size, and iteration all behave as expected.

#include <initializer_list>

int sum_and_check(std::initializer_list<int> values) {
    if (values.size() != 3)
        return 1;
    if (*values.begin() != 1)
        return 2;

    int sum = 0;
    for (const int* it = values.begin(); it != values.end(); ++it)
        sum += *it;

    return sum == 12 ? 0 : 3;
}

int main() {
    return sum_and_check({1, 4, 7});
}
