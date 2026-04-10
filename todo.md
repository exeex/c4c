# HIR Dense ID Maps And Storage Cleanup Todo

Status: Active
Source Idea: ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md
Source Plan: plan.md

## Active Item

- Step 4: record the landed `InlineCloneContext` migration and define the next
  bounded dense-ID helper follow-on
- Current slice: leave `InlineCloneContext::{local_map,block_map,expr_map}` on
  `OptionalDenseIdMap` and evaluate whether `param_to_local` should join the
  helper family or stay as a separate sparse parameter-binding table
- Iteration target: identify the next dense allocator-produced HIR/lowering map
  that benefits from checked dense-ID access without widening into
  string-keyed lowering state

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
- Added `frontend_hir_tests` and verified dense-ID helper behavior directly
- Full regression check is monotonic: `test_before.log` had `3360/3360`
  passing tests and `test_after.log` had `3361/3361` passing tests

## Next Intended Slice

- Decide whether `InlineCloneContext::param_to_local` is worth migrating as a
  local helper-family cleanup or whether the better next slice is a different
  allocator-produced HIR table with typed IDs on both sides
