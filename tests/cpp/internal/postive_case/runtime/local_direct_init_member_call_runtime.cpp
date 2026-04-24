// Runtime regression: in an out-of-class member definition, block-scope
// direct-init with an unqualified member call argument should stay on the
// direct-init path instead of being misread as a function declaration.
struct Box {
    bool IsPositive() const;
    int compute() const;
};

bool Box::IsPositive() const {
    return true;
}

int Box::compute() const {
    const bool positive(IsPositive());
    return positive ? 1 : 0;
}

int main() {
    Box box;
    return box.compute() == 1 ? 0 : 1;
}
