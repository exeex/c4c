// Reduced parser-boundary regression: a top-level forward tag declaration is
// bookkeeping-only and should not materialize a synthetic `Empty` node before
// the following declaration.
struct Forward;

int kept;
