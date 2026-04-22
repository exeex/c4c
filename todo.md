# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4.2
Current Step Title: Extract Function-Level Dispatch Context Assembly Behind Module Owners
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed the next packet for plan step 1.4.2 by extracting the
compare-driven/non-return dispatch fallback path out of
`render_defined_function(...)` in
`src/backend/mir/x86/codegen/module/module_emit.cpp` into the anonymous-namespace
`ModuleFunctionDispatchFallbackSupport` owner, which now centralizes the
compare-driven entry callback wiring, the local structural dispatch ladder, and
the shared prepared-control-flow contract fallback so the function renderer only
assembles module-owned support objects and delegates the non-return route.

## Suggested Next

Continue plan step 1.4.2 by extracting the remaining function-level dispatch
context assembly out of `render_defined_function(...)`, most likely the
prepared-query / ABI / `PreparedX86FunctionDispatchContext` setup that still
wires `asm_prefix`, return-register resolution, and minimal param-register
lookup inline before the helper owners run.

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
- `ModuleFunctionDispatchFallbackSupport` now owns the compare-driven entry and
  local structural fallback lane; keep future non-return routing and contract
  rewrite logic there instead of rebuilding that ladder inline.
- Preserve the legacy `x86::emit_prepared_module(...)` symbol until the
  supervisor retires that compatibility entry explicitly.
- `module_emit.hpp` did not need a new public seam for this packet; keep the
  extraction module-local unless the supervisor explicitly asks for a broader
  interface change.

## Proof

Step 1.4 module-support seam packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
