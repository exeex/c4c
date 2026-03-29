// Parser-debug regression: keep the best-failure stack informative for an
// expression-statement parse failure inside a block.

int main() {
    return +;
}
