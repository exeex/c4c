# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair Compare-Result Bool/Cast Materialization
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan review rewrote Step 2 after the post-`i64` local-slot investigation
showed the old Step 2.1 was oversized. The active runbook now splits the route
into one-family packets: Step 2.1 covers compare-result bool/cast
materialization at `match`, Step 2.2 covers float/HFA local-slot consumption
at `fa_hfa11`, and Step 2.3 remains the proof-and-rehoming checkpoint.

## Suggested Next

Take a fresh Step 2.1 executor packet against
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` that repairs only
the compare-result bool/cast materialization family. The packet should prove
that `match` and nearby same-family routes advance without bundling float/HFA
local-slot work or later call-boundary work into the same slice.

## Watchouts

- The `i64` local-slot seam is no longer the only blocker. `00204.c` still has
  at least two distinct local-slot families before the old call-boundary
  target: compare-result bool/cast materialization at `match` and float/HFA
  local-slot consumption at `fa_hfa11`.
- Step 2.1 is now explicitly only the bool/cast family. Do not fold float/HFA,
  pointer, or other scalar helper expansion into the same packet.
- The temporary trace used to identify the dropouts was removed. Keep the
  working tree free of investigation-only diagnostics.

## Proof

Existing blocked proof remains the latest evidence for this route reset:
`bash -lc "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log; test ${PIPESTATUS[0]} -eq 0"`.
That run built successfully, `backend_x86_handoff_boundary` passed, and
`c_testsuite_x86_backend_src_00204_c` still failed before the expected later
call-boundary miss. Follow-up investigation localized the remaining owned
dropouts to `match` (`CastInst` at `logic.end.8`) and `fa_hfa11`
(`LoadLocalInst` at `entry`). Proof log: `test_after.log`.
