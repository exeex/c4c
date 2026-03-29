// Parse-only regression: reserve `noexcept` and `nullptr` as keyword tokens
// without breaking the parser compatibility paths that still rely on them.
// RUN: %c4cll --parse-only %s

int accepts_ptr(const int* value = nullptr) noexcept(true);

struct widget {
    widget() noexcept = default;

    int ready() const noexcept {
        const int* value = nullptr;
        return value == nullptr;
    }
};

int main() {
    widget value;
    return value.ready() ? 0 : 1;
}
