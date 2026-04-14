// Pending target: placement/global-qualified new expression support.

typedef unsigned long size_t;

void* operator new(size_t, void*);

struct Box {
    int value;
    Box(int v) : value(v) {}
};

int main() {
    char storage[sizeof(Box)];
    Box* p = ::new ((void*)storage) Box(7);
    return p->value - 7;
}
