# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4.2
Current Step Title: Extract Function-Level Dispatch Context Assembly Behind Module Owners
Plan Review Counter: 7 / 6
# Current Packet

## Just Finished

Completed the next packet for plan step 1.4.2 by moving the remaining
declaration / empty-block contract gate out of `render_defined_function(...)`
and into `ModuleFunctionDispatchAssemblySupport`, so
`render_defined_function(...)` in
`src/backend/mir/x86/codegen/module/module_emit.cpp` into the anonymous-namespace
is now only the call-site wrapper around one module-owned dispatch-or-render
handoff layered on top of the existing
`ModuleFunctionDispatchAssemblySupport`,
`ModuleFunctionReturnSupport`, and
`ModuleFunctionDispatchFallbackSupport` seams.

## Suggested Next

Review whether step 1.4.2 is now thin enough to stop extracting from
`render_defined_function(...)` and move to the next reviewed module-owned
support packet, or whether the remaining wrapper should itself become a named
`module/` helper before closing this seam-shrinking pass.

## Watchouts

- Keep `prepared_module_emit.cpp` wrapper-thin; new whole-module orchestration
  logic should land in `module/module_emit.cpp` or sibling reviewed seams
  instead of drifting back into the compatibility surface.
- Keep `module/module_data_emit.*` focused on data and symbol publication; the
  next packet should keep peeling orchestration support out of
  `render_defined_function(...)` without moving instruction-shape or lowering
  decisions behind the data seam.
- `ModuleMultiDefinedSupport` and `ModuleFunctionPreparedQueries` now
  centralize module-local orchestration state; keep follow-on extractions
  aligned with those seams instead of reintroducing top-level lambdas.
- `ModuleFunctionReturnSupport` now owns the prepared return/home-move path;
  follow-on packets should reuse that owner instead of duplicating frame-size
  or return-bundle queries back inside `render_defined_function(...)`.
- `ModuleFunctionRenderSupport` now owns the return-vs-fallback branch and the
  single-block i32-shape contract ladder; if follow-on packets keep shrinking
  the lambda, preserve these error strings and contract annotations at the
  module-owned seam instead of drifting them back into function-local code.
- `ModuleFunctionDispatchFallbackSupport` now owns the compare-driven entry and
  local structural fallback lane; keep future non-return routing and contract
  rewrite logic there instead of rebuilding that ladder inline.
- `ModuleFunctionDispatchAssemblySupport` now owns the prepared-query / ABI /
  dispatch-context setup plus the defined-function contract gate; if the next
  packet widens that seam, preserve the callback-scoped lifetime model instead
  of returning self-referential support aggregates by value.
- Preserve the legacy `x86::emit_prepared_module(...)` symbol until the
  supervisor retires that compatibility entry explicitly.
- `module_emit.hpp` did not need a new public seam for this packet; keep the
  extraction module-local unless the supervisor explicitly asks for a broader
  interface change.

## Proof

Step 1.4.2 module-support seam packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
