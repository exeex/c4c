// Parser-debug regression: keep the inner record-member parameter failure when
// an outer record/top-level wrapper reaches the same EOF token later.

class Broken {
    struct Nested {
        int fn(int value = foo( ;
    };
};
