// Parser-debug regression: keep the type-like record-member dispatch visible in
// the summary stack when a malformed member alias target would otherwise unwind
// to the outer record-member wrapper.

class Broken {
    using Alias = Foo<;
};
