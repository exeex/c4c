# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire Prepared Adapters And Debug Surfaces Over Canonical Seams
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Continued step 3 by moving the prepared `I32` immediate-compare selection and
`eax` compare-setup helper family out of
`prepared_local_slot_render.cpp` and into
`prepared/prepared_fast_path_dispatch.{hpp,cpp}`, then deleting the duplicate
local definitions so the canonical prepared dispatch seam now owns both the
guard-compare adapter entrypoints and the shared immediate-compare queries they
consume.

## Suggested Next

Continue step 3 by moving the remaining local
`render_prepared_guard_false_branch_compare_with_current_i8_if_supported(...)`
wrapper and its branch-opcode selection into
`prepared/prepared_fast_path_dispatch.cpp` so prepared false-branch compare
ownership stops straddling the local-slot renderer and the canonical dispatch
seam, without widening into same-module call orchestration, `module_emit.cpp`,
or `route_debug.cpp`.

## Watchouts

- `prepared_fast_path_dispatch.{hpp,cpp}` now owns the prepared `I32`
  immediate-compare query/setup helpers in addition to the guard-compare entry
  points, so follow-on packets should extend that seam instead of recreating
  compare-selection helpers in `prepared_local_slot_render.cpp`.
- `prepared_local_slot_render.cpp` still keeps the `current_i8` false-branch
  compare wrapper plus larger local guard-expression and block-branch planning
  logic inline; keep peeling one coherent dispatch/helper family at a time
  instead of flattening the whole file at once.
- Keep ownership out of same-module call dispatch, `module/module_emit.cpp`,
  and `route_debug.cpp`; this slice stayed within prepared dispatch/query
  adaptation only.

## Proof

Step 3 prepared fast-path dispatch immediate-compare helper seam packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
