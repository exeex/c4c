// Parser-debug regression: keep simple range-for declaration probing on the
// lite rollback path and leave the later statement parse failure as the
// committed cause.

int values[2] = {1, 2};

int main() {
    for (const int value : values)
        return value + ;
    return 0;
}
