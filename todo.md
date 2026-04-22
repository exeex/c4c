# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1.4
Current Step Title: Materialize Module Support Seams While Preserving Compatibility Wrappers
Plan Review Counter: 4 / 6
# Current Packet

## Just Finished

Completed plan step 1.4 by extracting the module-level ABI/register support
helpers in `module/module_emit.cpp` into anonymous-namespace functions for
register narrowing, minimal parameter/return ABI register lookup, and
function-assembly prefix rendering, shrinking the top-level
`emit_prepared_module_text(...)` orchestration body without changing lowering
ownership or the legacy prepared-module compatibility entry behavior.

## Suggested Next

Continue plan step 1.4 by extracting the defined-function inventory and
multi-defined dispatch setup at the top of `module/module_emit.cpp` into the
next bounded module-support helper family so whole-module route selection keeps
moving out of the monolithic entry function without inventing a new reviewed
public seam.

## Watchouts

- Keep `prepared_module_emit.cpp` wrapper-thin; new whole-module orchestration
  logic should land in `module/module_emit.cpp` or sibling reviewed seams
  instead of drifting back into the compatibility surface.
- Keep `module/module_data_emit.*` focused on data and symbol publication; the
  next packet should peel route-selection support out of orchestration without
  moving instruction-shape or lowering decisions behind the data seam.
- The extracted anonymous-namespace helpers are implementation-local support,
  not a license to add a new reviewed public file boundary; preserve the stage-3
  contract unless lifecycle repair explicitly reopens it.
- Preserve the legacy `x86::emit_prepared_module(...)` symbol until the
  supervisor retires that compatibility entry explicitly.

## Proof

Step 1.4 module-support seam packet on 2026-04-22 using:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Proof passed. Proof log path: `test_after.log`
