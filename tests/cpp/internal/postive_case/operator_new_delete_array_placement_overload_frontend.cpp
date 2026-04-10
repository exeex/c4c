typedef unsigned long size_t;

void* operator new[](size_t);
void* operator new[](size_t, void*);

void operator delete[](void*);
void operator delete[](void*, void*);

int main() {
    return 0;
}
