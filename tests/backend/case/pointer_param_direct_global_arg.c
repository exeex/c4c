int g;

int consume(int *p) { return 0; }

int main(void) { return consume(&g); }
