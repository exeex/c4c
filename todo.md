# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed step 4's wrapper-classification packet by marking
`src/backend/mir/x86/codegen/emit.cpp` as the legacy public entry compatibility
shim for direct BIR/LIR/assemble callers and
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` as the legacy
prepared-module symbol shim, while leaving real module-entry ownership in the
reviewed `api/` and `module/` seams.

## Suggested Next

Audit the remaining step 4 public dispatch surfaces outside these wrappers,
especially the generic `backend.cpp` x86 entrypoints, and decide whether any
residual module-entry bounce paths still need explicit forwarding
classification.

## Watchouts

- `emit.cpp` still owns direct target-profile resolution and the direct-LIR
  rewrite-in-progress error remap; that is now explicit compatibility behavior,
  not a place to reintroduce lowering or module orchestration.
- `prepared_module_emit.cpp` remains intentionally thin and forwards straight to
  `module::emit_prepared_module_text(...)`; any new whole-module logic belongs
  under `module/`, not back in the wrapper.

## Proof

Step 4 wrapper-classification packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
