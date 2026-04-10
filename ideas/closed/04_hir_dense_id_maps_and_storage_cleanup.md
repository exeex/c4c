# HIR Dense ID Maps And Storage Cleanup

Status: Closed
Last Updated: 2026-04-10

## Goal

Replace obviously ID-shaped lookup patterns in HIR and frontend lowering with
denser storage helpers so code that works with `LocalId`, `BlockId`, `ExprId`,
and similar indices stops defaulting to hash maps when the data is naturally
index-addressable.

## Why This Idea Exists

The HIR layer already has explicit stable ID types:

- `LocalId`
- `BlockId`
- `ExprId`
- `GlobalId`
- `FunctionId`

That is a strong foundation. But a number of supporting maps still use
`std::unordered_map<uint32_t, T>` even when IDs are allocator-produced and
often dense within one module or one function.

That mismatch has a few costs:

- more boilerplate for lookup/update
- weaker invariants around "must exist" versus "may exist"
- harder future adoption of LLVM-like dense helpers or small-inline storage

## Main Objective

Introduce a frontend-owned dense ID map abstraction, then migrate the most
obvious HIR/lowering call sites that benefit from it.

The first step can be very small:

- `DenseIdMap<Id, T>`
- `OptionalDenseIdMap<Id, T>`
- helper APIs for checked access and sparse occupancy

The internal implementation can initially remain STL-backed if that keeps the
slice safer.

## Completion

Completed on 2026-04-10.

Landed scope:

- added frontend-owned `DenseIdMap<Id, T>` and `OptionalDenseIdMap<Id, T>`
- migrated `InlineCloneContext::{local_map, block_map, expr_map}` to
  `OptionalDenseIdMap`
- migrated `Lowerer::FunctionCtx::local_types` to `DenseIdMap<LocalId,
  TypeSpec>`
- migrated `run_inline_expansion`'s `callee_expand_count` recursion guard to
  `OptionalDenseIdMap<FunctionId, int>`
- extended `frontend_hir_tests` with direct helper coverage for `LocalId`,
  `ExprId`, and `FunctionId` storage patterns

Validation:

- targeted checks passed for `frontend_hir_tests` and
  `positive_sema_inline_phase9_c`
- full `ctest --test-dir build -j --output-on-failure` remained monotonic at
  `3361/3361` passing before and after the final recursion-guard migration

Deferred follow-on:

- keep string-keyed lowering maps out of scope
- keep `InlineCloneContext::param_to_local` as a parameter-index table unless a
  later slice shows that helper-backed checked access materially improves it
- next dense-ID candidate should be another function-local typed-ID map that
  still uses raw integer keys and clearly behaves like dense storage

## Primary Scope

- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/inline_expand.*`
- HIR/lowering support code that maps typed IDs to metadata
- selected `src/codegen/shared` or `src/codegen/lir` helpers that consume HIR IDs

## Candidate First Migrations

1. inline-expansion remap tables for locals/blocks/exprs
2. per-function lowering metadata keyed by local or block IDs
3. other obviously dense function-local maps where IDs are minted
   monotonically

## Constraints

- keep the first slice scoped to frontend / lowering code, not backend
- preserve API clarity; this is not a "micro-opt everything" project
- do not force dense storage onto genuinely sparse string-keyed data
- allow a staged migration where wrappers land before representation changes

## Validation

At minimum:

- existing HIR regression coverage
- inline expansion tests
- lowering/codegen tests that exercise the migrated ID-mapped metadata

## Non-Goals

- no backend allocator or register-allocation storage work
- no full replacement of all `unordered_map`
- no premature custom allocator work unless a migrated helper truly needs it
