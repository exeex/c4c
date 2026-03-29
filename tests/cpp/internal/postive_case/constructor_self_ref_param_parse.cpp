// Parse-only regression: unresolved C++ parameter types followed by lvalue or
// rvalue references should be treated as type spellings, not declarator names.
// RUN: %c4cll --parse-only %s

struct Box;

void accept(const Box& other, Box&& moved);

int main() {
    return 0;
}
