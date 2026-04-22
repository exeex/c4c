# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4.2
Current Step Title: Extract Function-Level Dispatch Context Assembly Behind Module Owners
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed a first packet for plan step 1.4.2 by extracting function-local
prepared-query lookup and `PreparedX86FunctionDispatchContext` assembly out of
`src/backend/mir/x86/codegen/module/module_emit.cpp` into an anonymous-namespace
`ModuleFunctionPreparedQueries` / `build_module_function_dispatch_context(...)`
helper pair, so `render_defined_function(...)` now delegates per-function
prepared metadata discovery and dispatch-context wiring without changing any
lowering decisions or widening the compatibility surface.

## Suggested Next

Continue plan step 1.4.2 by extracting the next bounded module-local support
cluster from `module_emit.cpp` inside `render_defined_function(...)`, most
likely the prepared return/home-move helpers
(`required_function_frame_size`, `append_i32_home_move`,
`render_frame_wrapped_return`, and adjacent return-bundle helpers), while
keeping the reviewed compatibility wrapper thin and avoiding any new public
seam.

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
- Preserve the legacy `x86::emit_prepared_module(...)` symbol until the
  supervisor retires that compatibility entry explicitly.

## Proof

Step 1.4 module-support seam packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
