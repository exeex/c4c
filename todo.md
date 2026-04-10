# HIR Dense ID Maps And Storage Cleanup Todo

Status: Active
Source Idea: ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md
Source Plan: plan.md

## Active Item

- Step 4: evaluate and, if justified, land the inline expansion recursion-guard
  migration
- Current slice: keep `InlineCloneContext::param_to_local` as a parameter-index
  table, add focused `FunctionId` helper coverage, and decide whether
  `run_inline_expansion`'s `callee_expand_count` should become
  `OptionalDenseIdMap<FunctionId, int>`
- Iteration target: prove the recursion guard behaves like dense typed-ID
  storage, then migrate that one call site without widening into unrelated
  frontend maps

## Completed Items

- Activated `plan.md` from
  `ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md`
- Step 1 inventory: confirmed the first dense-ID migration surface is
  `InlineCloneContext` in `src/frontend/hir/inline_expand.*`; nearby
  string-keyed lowering maps remain out of scope for this slice
- Added `DenseIdMap<Id, T>` and `OptionalDenseIdMap<Id, T>` to
  `src/frontend/hir/hir_ir.hpp`
- Migrated `InlineCloneContext::{local_map,block_map,expr_map}` in
  `src/frontend/hir/inline_expand.*` from raw `unordered_map<uint32_t, *>` to
  `OptionalDenseIdMap`
- Migrated `Lowerer::FunctionCtx::local_types` from
  `unordered_map<uint32_t, TypeSpec>` to `DenseIdMap<LocalId, TypeSpec>` across
  frontend lowering call sites
- Added `frontend_hir_tests` and verified dense-ID helper behavior directly
- Extended `frontend_hir_tests` with direct `TypeSpec`-backed `DenseIdMap`
  coverage for `LocalId` storage
- Full regression check is monotonic: `test_before.log` had `3360/3360`
  passing tests and the prior helper landing reached `3361/3361`; this
  `local_types` migration preserved `3361/3361` passing tests before and after

## Next Intended Slice

- Evaluate `run_inline_expansion`'s `callee_expand_count` as the next bounded
  frontend HIR dense-ID helper candidate; keep string-keyed lowering maps and
  `param_to_local` out of scope unless a later slice proves they benefit from
  helper-backed checked access
