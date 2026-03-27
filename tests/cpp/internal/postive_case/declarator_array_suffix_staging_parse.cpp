// Parse-only: declarator array suffix handling should stay stable across
// grouped declarators and parenthesized function-pointer declarators.
// RUN: %c4cll --parse-only %s

typedef int (*Handler)(int);

int forward(int value);

int (*callbacks[4])(int);
int (grouped_values[2])[3];
int (*nested_tables[2])[3];

struct Holder {
    int (*members[3])(int);
    int (grid[2])[4];
};

int accept_callbacks(int (*slots[4])(int), int (matrix[2])[3]) {
    (void)slots;
    (void)matrix;
    return 0;
}

int main() {
    Handler local = forward;
    callbacks[0] = local;
    return accept_callbacks(callbacks, grouped_values);
}
