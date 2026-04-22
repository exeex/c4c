# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4.3
Current Step Title: Classify Remaining Module-Orchestration Support Between `module_emit` And `module_data_emit`
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed plan step 1.4.3 by classifying the remaining top-of-file helper
inventory in `src/backend/mir/x86/codegen/module/module_emit.cpp` behind
module-local owners: `ModuleAssemblyInventorySupport` now owns whole-module
defined-function inventory and multi-defined dispatch assembly, while
`ModuleMinimalAbiSupport` and `ModuleFunctionPreparedQuerySupport` own the
function-lowering ABI/register and prepared-query/dispatch-context helpers
consumed by `ModuleFunctionDispatchAssemblySupport`.

## Suggested Next

Move to the next packet that verifies whether step 1.4.3 is complete and, if
so, continue the wrapper-thinning route around
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` without widening module
public seams.

## Watchouts

- Keep `prepared_module_emit.cpp` wrapper-thin; any remaining module-routing or
  compatibility work should stay under `module/` owners instead of drifting
  back into the legacy surface.
- Keep `module/module_data_emit.*` focused on data and symbol publication; new
  support helpers may delegate to `ModuleDataSupport`, but concrete
  `.rodata`/`.data`/`.bss` formatting must stay behind that seam.
- `ModuleAssemblyInventorySupport` is now the top-of-file whole-module owner;
  keep future module inventory or multi-defined orchestration helpers there
  instead of reintroducing free helpers.
- `ModuleMinimalAbiSupport` and `ModuleFunctionPreparedQuerySupport` are now
  the function-lowering-side helper clusters; keep register narrowing,
  prepared-query lookup, block lookup, and dispatch-context assembly on that
  side of the seam.
- `ModuleRenderSupport` still owns final whole-module render/finalization
  choices after assembly, and `ModuleMultiDefinedSupport` still owns bounded
  helper state and contract detail for multi-defined modules.
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
