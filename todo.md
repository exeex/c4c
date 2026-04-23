# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 8 / 6
# Current Packet

## Just Finished

Completed a step 4 backend-front-door classification packet by splitting the
x86 BIR public handoff into an explicit `emit_x86_bir_module_entry(...)`
surface while keeping `emit_target_bir_module(...)` as the compatibility
wrapper for non-x86 prepared-BIR callers. Updated the focused x86
handoff-boundary tests so the generic backend emit path now proves it delegates
through the explicit x86 BIR entry instead of the legacy generic name.

## Suggested Next

Audit the remaining step 4 public entrypoints and downstream target shims for
generic names that still hide x86-only ownership, especially any
`emit_target_bir_module(...)` compatibility call sites in backend-facing target
wrappers that can now move to the explicit x86 BIR entry surface.

## Watchouts

- `emit_target_bir_module(...)` still exists as the shared compatibility
  wrapper, so any follow-on packet that rewires downstream x86 callers has to
  keep the non-x86 prepared-BIR contract intact.
- `src/backend/mir/aarch64/codegen/emit.cpp` still calls the compatibility
  `emit_target_bir_module(...)` surface; step 4 cleanup should distinguish
  honest non-x86 compatibility use from x86-only ownership that can now route
  through `emit_x86_bir_module_entry(...)`.

## Proof

Step 4 backend-front-door x86 BIR entry classification packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
