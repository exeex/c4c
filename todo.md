# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Shift Module Entry And Legacy Dispatch To The Replacement Graph
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed step 4's first public-entry routing packet by moving
`api/x86_codegen_api.cpp` off the generic `emit_target_*` bounce path, so the
x86 API now performs its own BIR preparation / LIR-to-BIR lowering and then
hands directly into `emit_prepared_module(...)` and
`module::emit_prepared_module_text(...)`.

## Suggested Next

Classify the remaining legacy public wrappers around the x86 module-entry path,
starting with whether `emit.cpp` and `prepared_module_emit.cpp` should stay as
explicit compatibility forwarders or can shrink further now that the API owns
preparation and handoff.

## Watchouts

- `backend.cpp` still has the generic cross-target `emit_target_*` entrypoints;
  this packet only removed the x86 API's dependency on bouncing back through
  that generic layer.
- `emit.cpp` still owns direct target-profile resolution and the compatibility
  rewrite-in-progress error remap for direct LIR callers.

## Proof

Step 4 api-to-module handoff packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
