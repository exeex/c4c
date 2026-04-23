// Parser-debug regression: keep simple `while` declaration-condition probing on
// the lite rollback path and leave the later statement parse failure as the
// committed cause.

int main() {
    while (const int value = 1)
        return value + ;
    return 0;
}
