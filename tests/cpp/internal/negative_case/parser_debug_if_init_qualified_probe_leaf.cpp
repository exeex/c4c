// Parser-debug regression: keep the committed parser leaf when an unsupported
// `if` init-statement triggers a speculative qualified-type probe under
// `parse_primary`.

using size_t = unsigned long;

int main() {
    if (const size_t len = 1)
        return len;
    return 0;
}
