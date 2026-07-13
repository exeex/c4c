# Runtime Helper Plan

Status: scaffold. Selects declared helper interfaces for target-unsupported
semantic operations without using testcase-shaped matching. MIR lowering emits
the calls according to the ABI and call plans.

Legacy coverage: i128/f128 runtime helpers, atomic helpers, intrinsic helpers,
symbol declarations, clobbers, returns, and helper recursion guards.

Exact audit anchors include `i128_runtime_helpers.*`,
`f128_runtime_helpers.*`, and `regalloc/runtime_helpers.*`.
