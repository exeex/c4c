int* g_base = 0;
int* g_slot = 0;

int*& slot_ref() {
    return g_slot;
}

void assign_slot(int offset) {
    slot_ref() = g_base + offset;
}

int main() {
    return 0;
}
