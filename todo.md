# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4.3
Current Step Title: Classify Remaining Module-Orchestration Support Between `module_emit` And `module_data_emit`
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed the next packet for plan step 1.4.3 by extracting the remaining
defined-function inventory and multi-defined dispatch assembly in
`src/backend/mir/x86/codegen/module/module_emit.cpp` into the anonymous-namespace
`ModuleAssemblySupport` owner, so `emit_prepared_module_text(...)` now mainly
validates the target, creates module data support, and delegates the rest of
whole-module routing through one module-owned seam while `module_data_emit.*`
continues to own final data and symbol publication.

## Suggested Next

Continue plan step 1.4.3 by deciding whether the remaining helper inventory at
the top of `module_emit.cpp` should be explicitly classified between
whole-module orchestration support and function-lowering support, or whether
the step is now complete enough to move on to wrapper classification around
`prepared_module_emit.cpp`.

## Watchouts

- Keep `prepared_module_emit.cpp` wrapper-thin; any remaining module-routing or
  compatibility work should stay under `module/` owners instead of drifting
  back into the legacy surface.
- Keep `module/module_data_emit.*` focused on data and symbol publication; new
  support helpers may delegate to `ModuleDataSupport`, but concrete
  `.rodata`/`.data`/`.bss` formatting must stay behind that seam.
- `ModuleAssemblySupport` now owns defined-function inventory plus
  multi-defined dispatch setup; if the next packet widens that seam, keep it
  about whole-module orchestration rather than function-lowering detail.
- `ModuleRenderSupport` still owns the final whole-module render/finalization
  choices after assembly; avoid folding function-level prepared-query or
  return-lowering detail back into that seam.
- `ModuleMultiDefinedSupport` still centralizes bounded helper state and
  contract detail for multi-defined modules; reuse it instead of rebuilding
  helper-prefix or contract annotations in top-level routing code.
- `ModuleFunctionDispatchAssemblySupport` still owns per-function prepared-query
  / ABI / dispatch-context setup plus the concrete render entrypoint; follow-on
  module packets should not pull those concerns back into whole-module routing.
- Preserve the legacy `x86::emit_prepared_module(...)` symbol until the
  supervisor retires that compatibility entry explicitly.
- `module_emit.hpp` still does not need a broader public seam for this packet;
  keep these classifications module-local unless the supervisor explicitly
  needs a wider interface change.

## Proof

Step 1.4.3 module-support seam packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
