static int retained_static_literal(void) { return 31; }

int (*retained_static_anchor)(void) = retained_static_literal;

int ordinary_external_literal(void) { return 7; }
