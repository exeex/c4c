// Parser-debug regression: keep simple `if` declaration-condition probing on
// the lite rollback path and leave the later statement parse failure as the
// committed cause.

int main() {
    if (const int value = 1)
        return value + ;
    return 0;
}
