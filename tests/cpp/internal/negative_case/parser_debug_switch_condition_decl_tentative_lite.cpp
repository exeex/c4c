// Parser-debug regression: keep simple `switch` declaration-condition probing on
// the lite rollback path and leave the later statement parse failure as the
// committed cause.

int main() {
    switch (const int value = 1) {
        default:
            return value + ;
    }
}
