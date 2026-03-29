// Parse-only regression: malformed record-member requires-clauses should stop
// before the next obvious member declaration instead of consuming its type as
// another constraint atom.

struct Broken {
  template<typename T>
  requires std::is_same_v<T, int> &&
  int kept;
};
