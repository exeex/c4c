// Parser-debug regression: keep malformed record typedef targets pinned to the
// typedef-member parser instead of collapsing back to the type-like wrapper.

class Broken {
    typedef Foo<Bar Alias;
};
