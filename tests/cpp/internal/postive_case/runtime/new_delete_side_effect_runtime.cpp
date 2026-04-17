typedef unsigned long size_t;

static int g_new_calls = 0;
static int g_delete_calls = 0;
static unsigned char g_storage[64];

void* operator new(size_t) {
    g_new_calls += 1;
    return (void*)g_storage;
}

void operator delete(void*) {
    g_delete_calls += 1;
}

struct Box {
    int value;
};

int main() {
    Box* p = new Box;
    p->value = 7;
    delete p;

    if (g_new_calls != 1) return 1;
    if (g_delete_calls != 1) return 2;
    return 0;
}
