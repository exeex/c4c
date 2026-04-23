# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire Prepared Adapters And Debug Surfaces Over Canonical Seams
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Completed step 3's prepared fast-path dispatch seam materialization by adding
`prepared/prepared_fast_path_dispatch.{hpp,cpp}`, moving the guarded prepared
compare/false-branch helper family out of
`prepared_param_zero_render.cpp`, rewiring the prepared local-slot and
param-zero callers to include that seam explicitly, and keeping the move
bounded away from module/debug ownership.

## Suggested Next

Continue step 3 by moving the next reviewed prepared adapter/debug helper group
that still hangs off the legacy top-level prepared surfaces into the canonical
prepared seam without widening into `module_emit.cpp` or `route_debug.cpp`.

## Watchouts

- `prepared_fast_path_dispatch.hpp` currently owns only the guarded compare and
  false-branch dispatch helpers; do not use this packet as cover to move the
  broader param-zero entry renderer or short-circuit planner wholesale.
- `x86_codegen.hpp` still intentionally carries the shared compare-selection
  declarations consumed across prepared seams; the new seam should stay narrow
  until the reviewed follow-up packet moves more ownership.
- Keep ownership out of `module/module_emit.cpp` and `route_debug.cpp`; this
  slice proved the caller rewires without touching either forbidden surface.

## Proof

Step 3 prepared fast-path dispatch guarded compare/branch seam packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
