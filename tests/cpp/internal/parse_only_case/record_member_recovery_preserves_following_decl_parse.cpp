// Parse-only regression: malformed record members should recover at the next
// simple field declaration instead of silently swallowing it.

struct Broken {
  ~Broken(
  int kept;
};
