// Probe: assignment to the result of a function returning a reference.

int* g_begin = 0;
int* g_capacity = 0;

int*& capacity_ptr() {
    return g_capacity;
}

void reserve_like(int n) {
    capacity_ptr() = g_begin + n;
}

int main() {
    return 0;
}
