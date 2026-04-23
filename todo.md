# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Completed a step 4 module-entry classification packet by splitting the x86 LIR
front door into explicit public `emit_x86_lir_module_entry(...)` and
`stage_x86_lir_module_entry(...)` surfaces, while keeping
`emit_target_lir_module(...)` and `assemble_target_lir_module(...)` as
compatibility wrappers that preserve the current generic/bootstrap contract for
non-x86 callers. Added focused x86 handoff-boundary assertions proving the new
explicit entrypoints still match the canonical x86 prepared-module and staging
seams, and that the compatibility wrappers preserve the same rejection and
bootstrap-result contracts.

## Suggested Next

Audit the remaining step 4 public entrypoints in `src/backend/backend.cpp` and
`src/backend/backend.hpp` for any other legacy x86-only dispatch that still
sits behind generic naming, especially around BIR/public entry surfaces that
still combine target-local ownership with shared backend compatibility names.

## Watchouts

- `emit_target_lir_module(...)` and `assemble_target_lir_module(...)` still
  exist as compatibility entrypoints because non-x86 callers in the tree route
  through those names today; a future packet can only retire or rename them if
  it updates those downstream call sites as part of the same slice.
- `emit_target_bir_module(...)` still mixes a generic prepared-BIR fallback
  with x86 prepared-module emission under one public name; if step 4 continues,
  that BIR entry cluster is the next obvious ownership boundary to classify.

## Proof

Step 4 backend-front-door module-entry classification packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
