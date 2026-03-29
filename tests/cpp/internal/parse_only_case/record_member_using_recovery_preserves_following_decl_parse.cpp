// Parse-only regression: malformed record using-members should recover at the
// next simple field declaration instead of silently swallowing it.

struct Base {
  using Value = int;
};

struct Broken : Base {
  using Base::
  int kept;
};
