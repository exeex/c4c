# Plan Todo

Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

## Active Item

- Step 1: Establish Lowering Entrypoints

## Incomplete Items

- [ ] Step 1: inspect the current `adapt_minimal_module()` API surface and
      introduce a lowering-named entrypoint without behavior change
- [ ] Step 2: separate legacy LIR syntax decode helpers from backend IR
      construction
- [ ] Step 3: convert the highest-value backend IR text-carried semantics into
      structured representations
- [ ] Step 4: remove at least one target-codegen dependency on
      `parse_backend_*` helpers
- [ ] Step 5: run targeted and full regression validation for each landed slice

## Completed Items

- [x] Activated `ideas/open/03_lir_to_backend_ir_refactor.md` into `plan.md`
- [x] Initialized `plan_todo.md` with matching source metadata
- [x] Step 1 slice: expose `lower_to_backend_ir(...)` directly from the
      `lir_adapter` boundary while keeping `adapt_minimal_module(...)` as a
      compatibility shim

## Next Intended Slice

- Move remaining direct `adapt_minimal_module()` call sites and adjacent tests
  onto `lower_to_backend_ir(...)`, then start extracting decode helpers out of
  `lir_adapter.cpp`.

## Blockers

- None recorded.

## Resume Notes

- Keep this slice behavior-preserving.
- Do not pull `bir` naming into this plan.
- `lower_to_backend_ir(...)` now lives in `src/backend/lir_adapter.hpp` and
  `src/backend/lir_adapter.cpp`; `adapt_minimal_module(...)` remains a
  compatibility shim for callers that have not moved yet.
- If the work expands into a separate migration initiative, record it under
  `ideas/open/` instead of widening the active runbook.
