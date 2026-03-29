// Parser-debug regression: keep the summary stack informative for a malformed
// nested record member even after record-member dispatch rewinds.

class Broken {
    struct Nested {
        int value;
};
