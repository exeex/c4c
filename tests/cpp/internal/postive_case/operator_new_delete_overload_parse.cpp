typedef unsigned long size_t;

struct Arena {
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};

void* Arena::operator new(size_t) { return 0; }
void Arena::operator delete(void*) {}
void* Arena::operator new[](size_t) { return 0; }
void Arena::operator delete[](void*) {}

void* operator new(size_t);
void operator delete(void*);
void* operator new[](size_t, const char*, int, unsigned, const char*, int);
void operator delete[](void*);

void test_explicit_operator_calls() {
    void* p = ::operator new(8);
    ::operator delete(p);
    void* q = ::operator new[](16, "arena", 0, 0, "file", 7);
    ::operator delete[](q);
}

int main() { return 0; }
