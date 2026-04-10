// Parser-debug regression: keep expression-like constructor-style direct-init
// probing on the lite rollback path and leave the later expression failure as
// the committed cause.

struct Box {
    Box(int value);
    int value;
};

int main() {
    Box item(1 + 2);
    return item.value + ;
}
