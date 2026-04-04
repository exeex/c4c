#pragma c4 exec host
int host_fn(void) { return 11; }

#pragma c4 exec device
int device_fn(void) { return 22; }

#pragma c4 exec host
int main(void) { return host_fn() - 11; }
