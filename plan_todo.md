# Plan Todo

Status: Active
Source Idea: ideas/open/06_bir_cutover_and_legacy_cleanup.md
Source Plan: plan.md

## Active Item

- Step 2: Flip the default to BIR with tests first.

## In Progress

- Capture the first safe default-flip slice now that the routing surface is
  mapped.
- Determine whether the next change should extend the BIR scaffold beyond the
  current tiny-slice lowering or thread an explicit temporary fallback through
  the main codegen entrypoint.

## Pending

- Step 3: Remove legacy backend plumbing that is no longer exercised.
- Step 4: Clean up surviving transitional `backend_ir` naming.
- Step 5: Finish backend test migration around the BIR-first path.

## Completed

- Activated the runbook from
  `ideas/open/06_bir_cutover_and_legacy_cleanup.md`.
- Step 1 cutover-surface inventory:
  `src/backend/backend.hpp` still defaults `BackendOptions.pipeline` to
  `BackendPipeline::Legacy`.
- `src/backend/backend.cpp` only consults `options.pipeline` on the LIR entry
  path where `BackendModuleInput.module == nullptr`; once callers pass a
  lowered backend module, routing no longer branches on BIR-vs-legacy.
- `src/codegen/llvm/llvm_codegen.cpp` currently builds default backend options
  and never opts into the BIR pipeline, so production codegen still enters the
  legacy route by default.
- Added backend pipeline tests that pin the LIR-entry seam as the only current
  BIR selection surface.

## Notes

- Keep execution backend-focused; do not broaden this into unrelated cleanup.
- Preserve a fallback only if current failures show it is still required.
- Use backend-only targeted validation before broader full-suite regression
  checks.
- Current blocker to an unconditional default flip: `lower_to_bir(...)` still
  only accepts narrow single-block i32 return-immediate/add-sub slices.

## Next Slice

- Choose one minimal Step 2 change that moves the default route closer to BIR
  without widening beyond the current scaffold support.
- Likely candidates: thread an explicit temporary backend-pipeline switch
  through `emit_module_native(...)`, or broaden `lower_to_bir(...)` enough to
  cover a real defaulted caller slice first.

## Blockers

- `lower_to_bir(...)` is still too narrow for a safe global default flip.
