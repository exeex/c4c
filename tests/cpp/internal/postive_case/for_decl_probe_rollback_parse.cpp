// Parser regression: regular `for` declarations must still parse after the
// range-for tentative probe rolls back from `:` to `;`.

int accumulate(int limit) {
    int total = 0;
    for (int i = 0; i < limit; ++i) {
        total = total + i;
    }
    return total;
}
