// Pending target: class-specific operator new/delete lookup for expressions.

typedef unsigned long size_t;

struct PoolBox {
    int value;

    static void* operator new(size_t);
    static void operator delete(void*);
};

void* PoolBox::operator new(size_t) { return 0; }
void PoolBox::operator delete(void*) {}

int main() {
    PoolBox* p = new PoolBox;
    delete p;
    return 0;
}
