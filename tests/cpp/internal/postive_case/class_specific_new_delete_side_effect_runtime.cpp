typedef unsigned long size_t;

static int g_class_new_calls = 0;
static int g_class_delete_calls = 0;
static unsigned char g_class_storage[64];

struct PoolBox {
    int value;

    static void* operator new(size_t) {
        g_class_new_calls += 1;
        return (void*)g_class_storage;
    }

    static void operator delete(void*) {
        g_class_delete_calls += 1;
    }
};

int main() {
    PoolBox* p = new PoolBox;
    p->value = 11;
    delete p;

    if (g_class_new_calls != 1) return 1;
    if (g_class_delete_calls != 1) return 2;
    return 0;
}
