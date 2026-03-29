// Parser-debug regression: keep malformed record using-alias targets pinned to
// the alias-member parser instead of collapsing back to the type-like wrapper.

class Broken {
    using Alias = Foo<Bar;
};
