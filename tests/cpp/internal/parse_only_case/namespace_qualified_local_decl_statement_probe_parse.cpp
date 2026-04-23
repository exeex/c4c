// RUN: %c4cll --parse-only %s

namespace ns {
struct Type {
    int value;
};

int value() {
    return 7;
}
}  // namespace ns

int probe() {
    ns::Type local;
    return ns::value() + local.value;
}
