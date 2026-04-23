# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire Prepared Adapters And Debug Surfaces Over Canonical Seams
Plan Review Counter: 6 / 6
# Current Packet

## Just Finished

Continued step 3 by moving the remaining prepared `current_i8` false-branch
compare fast path and its branch-opcode selection out of
`prepared_local_slot_render.cpp` and into
`prepared/prepared_fast_path_dispatch.{hpp,cpp}`, then deleting the local
wrapper so canonical prepared dispatch now owns both the shared compare
selection helpers and the `cmp al, imm` guard-compare adapter path they use.

## Suggested Next

Continue step 3 by peeling the next compare-driven branch assembly helper out of
`prepared_local_slot_render.cpp` and into
`prepared/prepared_fast_path_dispatch.cpp`, preferably the short-circuit /
compare-join context selection that still reconstructs dispatch-owned compare
inputs inline after the canonical false-branch compare helper now owns all
guard-compare selection cases.

## Watchouts

- `prepared_fast_path_dispatch.{hpp,cpp}` now owns the prepared `I8`,
  prepared `I32`, and materialized-compare false-branch selection paths, so
  follow-on packets should thread new compare-context needs through that seam
  instead of reintroducing opcode selection in `prepared_local_slot_render.cpp`.
- `prepared_param_zero_render.cpp` now passes explicit `std::nullopt`
  `current_i8` context into the canonical helper API; keep that file limited to
  dispatch consumption rather than growing a second `current_i8` specialization.
- Keep ownership out of same-module call dispatch, `module/module_emit.cpp`,
  and `route_debug.cpp`; this slice stayed within prepared dispatch/query
  adaptation only.

## Proof

Step 3 prepared fast-path dispatch `current_i8` false-branch compare seam
packet on
2026-04-23:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
