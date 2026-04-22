# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.5
Current Step Title: Audit Transitional Forwarding And Buildability Across Shared Seams
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed a step-1.5 transitional-forwarding audit packet by making
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` an even thinner
compatibility wrapper and retargeting route-debug "next inspect" guidance away
from the legacy wrapper and toward
`src/backend/mir/x86/codegen/module/module_emit.cpp`, which now owns the
top-level prepared-module orchestration surfaced by the reviewed rebuild docs.

## Suggested Next

Continue step 1.5 by auditing the remaining shared-seam forwarding points under
`src/backend/mir/x86/codegen/`, especially any other legacy-facing surfaces or
debug hints that still imply ownership lives outside the reviewed `api/` and
`module/` seams.

## Watchouts

- Keep `prepared_module_emit.cpp` wrapper-thin; any remaining module-routing or
  compatibility work should stay under `module/` owners instead of drifting
  back into the legacy surface.
- Keep route-debug guidance pointed at the real owner for top-level module
  orchestration; only lane-specific hints that still genuinely live in
  `prepared_*` files should mention those surfaces.
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

Step 1.5 transitional-forwarding audit packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
