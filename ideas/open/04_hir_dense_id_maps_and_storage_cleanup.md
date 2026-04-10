# HIR Dense ID Maps And Storage Cleanup

Status: Open
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
