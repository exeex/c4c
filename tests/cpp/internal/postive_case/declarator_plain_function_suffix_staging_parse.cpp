// Parse-only: plain `name(params)` declarator suffixes should stay attached to
// the surrounding declaration while coordinator extraction keeps parameter-site
// function decay separate.
// RUN: %c4cll --parse-only %s

typedef int Unary(int);

int increment(int value);
int double_up(int value);

struct Runner {
    int method(int action(int), Unary alias_action);
    static int dispatch(int action(int), int fallback(int));
};

int accept_named(int callback(int), Unary alias_callback);
int accept_abstract(int(int), Unary);

int Runner::method(int action(int), Unary alias_action) {
    return action(3) + alias_action(4);
}

int Runner::dispatch(int action(int), int fallback(int)) {
    return action(5) + fallback(6);
}

int accept_named(int callback(int), Unary alias_callback) {
    return callback(7) + alias_callback(8);
}

int main() {
    Runner runner;
    return runner.method(increment, double_up) +
           Runner::dispatch(increment, double_up) +
           accept_named(increment, double_up) +
           accept_abstract(increment, double_up);
}
