/* Test Phase 9: expansion coverage improvements.
   Verifies that:
   - array-typed parameters are correctly decayed to pointers during inlining
   - mutual recursion between always_inline functions is handled safely
   - max inline depth is respected */

/* --- Array parameter decay --- */

static inline __attribute__((always_inline))
int sum_arr(int arr[], int n) {
    int s = 0;
    for (int i = 0; i < n; ++i) s += arr[i];
    return s;
}

static inline __attribute__((always_inline))
int first_elem(int a[1]) {
    return a[0];
}

static inline __attribute__((always_inline))
void set_arr(int dst[], int idx, int val) {
    dst[idx] = val;
}

/* --- Mutual recursion guard --- */
/* These should NOT cause infinite expansion; the inliner should detect
   mutual recursion and leave remaining calls un-expanded. */

static inline __attribute__((always_inline))
int mutual_a(int n);

static inline __attribute__((always_inline))
int mutual_b(int n) {
    if (n <= 0) return 0;
    return n + mutual_a(n - 1);
}

static inline __attribute__((always_inline))
int mutual_a(int n) {
    if (n <= 0) return 0;
    return n + mutual_b(n - 1);
}

int main(void) {
    /* Test 1: sum array via inline with array param */
    int data[3] = {10, 20, 30};
    int s = sum_arr(data, 3);
    if (s != 60) return 1;

    /* Test 2: single-element array param */
    int one[1] = {42};
    if (first_elem(one) != 42) return 2;

    /* Test 3: void callee with array param */
    int buf[2] = {0, 0};
    set_arr(buf, 0, 100);
    set_arr(buf, 1, 200);
    if (buf[0] != 100) return 3;
    if (buf[1] != 200) return 4;

    /* Test 4: mutual recursion should not crash/hang */
    int mr = mutual_a(3);
    /* mutual_a(3) = 3 + mutual_b(2) = 3 + (2 + mutual_a(1)) = 3 + (2 + (1 + mutual_b(0))) = 3+2+1+0 = 6 */
    if (mr != 6) return 5;

    return 0;
}
