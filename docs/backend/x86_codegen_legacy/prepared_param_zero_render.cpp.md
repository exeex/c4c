# `prepared_param_zero_render.cpp`

Primary role: prepared-route specializer for compare-against-zero and compare-against-immediate guard families.

Key surfaces:

```cpp
std::optional<std::string> render_prepared_param_zero_branch_prefix(...);
std::optional<std::pair<std::string, std::string>> render_prepared_guard_false_branch_compare(...);
std::optional<DirectBranchTargets> resolve_direct_branch_targets(...);
```

- Looks up prepared branch-condition and value-home facts, narrows register names locally, and emits compare/branch sequences for supported guard shapes.
- Owns predicate shaping that the canonical path would normally keep in `comparison.cpp`.
- Couples directly to prepared control-flow and branch-condition metadata rather than reusing one shared terminator-lowering seam.
- Special-case classification: `optional fast path` when bounded and thin; `overfit to reject` if additional narrow guard matchers keep accumulating here.
