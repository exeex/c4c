typedef unsigned long size_t;

static int g_new_array_calls = 0;
static int g_delete_array_calls = 0;
static unsigned char g_array_storage[128];

void* operator new[](size_t) {
    g_new_array_calls += 1;
    return (void*)g_array_storage;
}

void operator delete[](void*) {
    g_delete_array_calls += 1;
}

int main() {
    int* p = new int[4];
    p[0] = 3;
    p[3] = 9;
    delete[] p;

    if (g_new_array_calls != 1) return 1;
    if (g_delete_array_calls != 1) return 2;
    return 0;
}
