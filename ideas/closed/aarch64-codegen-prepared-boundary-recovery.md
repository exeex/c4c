# AArch64 Codegen Prepared Boundary Recovery

## Intent

`src/backend/mir/aarch64/codegen` currently owns too much Prepared/MIR bridge
logic. Before merging implementation files, identify which responsibilities are
not truly AArch64 instruction selection and move them back toward `bir`,
`prealloc`, or shared MIR support.

The goal is to make AArch64 codegen consume a clearer target contract instead of
repairing or interpreting broad Prepared state locally.

## Background

The reference prototype under
`ref/claudes-c-compiler/src/backend/arm/codegen` has far fewer architecture
codegen files because much of the shared algorithm lives outside the target
module. In this repo, `src/backend/prealloc/module.hpp` already carries rich
Prepared facts, but AArch64 still has large local families such as:

- `dispatch_*.cpp`
- `dispatch_*.hpp`
- `calls_*.cpp`
- `calls_*.hpp`

Some of that code is real AArch64 lowering. Some appears to be generic contract
interpretation: value homes, storage encodings, edge copies, call boundary
moves, publication effects, and diagnostic plumbing.

## Work

Audit the AArch64 codegen families and classify each helper/function into one
of these buckets:

- target-specific instruction selection or printing; should stay in AArch64
- generic Prepared contract interpretation; candidate for `prealloc` or shared
  MIR support
- missing Prepared fact; candidate to be computed earlier in `bir` or
  `prealloc`
- purely local publication/helper glue; candidate for consolidation after the
  boundary is clearer

Then move one narrow, low-risk generic responsibility out of
`src/backend/mir/aarch64/codegen` as a proof slice.

## Constraints

- Do not weaken tests, expectations, or backend contracts to make the move pass.
- Do not introduce target-specific special cases into BIR/prealloc.
- Do not merge large `.cpp` files in this idea; this idea is about ownership
  boundaries.
- Prefer moving facts earlier only when multiple targets or MIR layers could
  use them.

## Acceptance

- There is a written classification of the current `dispatch_*` and `calls_*`
  responsibilities in `todo.md` or a review artifact.
- At least one generic Prepared/MIR responsibility is moved out of AArch64
  codegen, or the audit justifies why the first move should be deferred.
- Fresh build or targeted backend tests prove the moved slice.

## Closure

Closed after Step 3 review accepted the boundary recovery slice. The audit
classified the `dispatch_*` and `calls_*` families, the Prepared value-home/name
lookup responsibility moved into shared `prealloc` support, and backend proof
remained stable at 149/149 with no new failures. Header-family consolidation is
unblocked; additional Prepared boundary refinement is optional future work, not
a prerequisite for that follow-on.
