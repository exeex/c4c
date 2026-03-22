// Pending target: plain new/delete expressions should resolve through
// user-provided operator new/delete without assuming a stdlib.

typedef unsigned long size_t;

void* operator new(size_t);
void operator delete(void*);

struct Box {
    int value;
};

int main() {
    Box* p = new Box;
    delete p;
    return 0;
}
