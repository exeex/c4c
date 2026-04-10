// Parser-debug regression: keep the sizeof(type) tentative probe on the lite
// rollback path and leave the later expression failure as the committed cause.

int main() {
    return sizeof(int) + ;
}
