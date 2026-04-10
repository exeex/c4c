# HIR Dense ID Maps And Storage Cleanup Todo

Status: Active
Source Idea: ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md
Source Plan: plan.md

## Active Item

- Step 1: inventory typed-ID keyed lookup surfaces in HIR and frontend lowering
  code, then choose the narrowest dense-ID migration slice with clear
  validation coverage
- Current slice: inspect `hir_ir.hpp`, `inline_expand.*`, and nearby lowering
  helpers for `LocalId`, `BlockId`, `ExprId`, `GlobalId`, and `FunctionId`
  maps that are allocator-produced and dense in practice
- Iteration target: define the first helper landing zone and migration boundary
  before any implementation edits

## Completed Items

- Activated `plan.md` from
  `ideas/open/04_hir_dense_id_maps_and_storage_cleanup.md`

## Next Intended Slice

- Add the minimal dense-ID helper types once the first migration surface is
  explicitly bounded
