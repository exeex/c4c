# Execution State

Status: Active
Source Idea Path: ideas/open/63_x86_backend_runtime_correctness_regressions.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Runtime Seam
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 2 packet stayed inside
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` and tested whether
the older i32 stack-home-sync helper could be reduced inside the accepted
`00040` fix. The helper is not a blanket no-effect experiment anymore, but it
also did not need both old call sites: removing the minimal local-slot return
use keeps the broader proof green, while the helper remains necessary on the
general scalar-load path that feeds the repaired same-module runtime flow. The
current worktree therefore narrows the helper to the smallest proven subset
needed by the accepted same-module callee-saved preservation fix, and the code
path is now a coherent Step 2 slice rather than a mixed cleanup experiment.

## Suggested Next

Keep Step 2 on runtime correctness, but move the next packet to the next owned
runtime case now that `00040` has a commit-ready repair. The next slice should
retarget idea 63 to `c_testsuite_x86_backend_src_00086_c` or `00130_c`,
starting with whichever one shares the closest same-module call or prepared
load/store seam with this accepted fix.

## Watchouts

- Keep ownership on runtime correctness; do not reopen idea 61's prepared
  call-bundle fallback path now that `00040` crosses codegen successfully.
- Keep the accepted fix scoped to same-module call participants. A first cut
  that saved callee-saved homes for every prepared guard-chain function made
  `c_testsuite_x86_backend_src_00040_c` pass but regressed
  `backend_x86_handoff_boundary`'s canonical short-circuit asm.
- The accepted route is function-level save/restore around the existing
  prepared guard-chain frame, not a mid-function `rsp` adjustment and not a
  renderer-local register swap around one call site.
- The current `00040` fix depends on preserving both the borrowed same-module
  scratch registers and any authoritative prepared home already placed in
  callee-saved registers for functions in that call family.
- Keep the existing i32-load home sync; that earlier fix is still real.
- The stack-home-sync helper is only proven necessary on the general scalar
  load path. The minimal local-slot return use was removed and the broader
  proof stayed green, so do not reintroduce that extra call site without fresh
  evidence.
- Do not revive the earlier broad binary/select i32 result sync experiment; it
  already proved to be a no-effect route.
- The frame-sizing and same-module call-preservation repairs from prior packets
  are still real prerequisites; do not regress those while chasing the new
  wrong-result seam.
- `00040` now passes under the broader backend subset, so the next packet
  should spend diagnosis budget on the remaining idea-63 cases instead of
  reopening this caller/callee preservation seam without fresh contrary proof.

## Proof

Ran the active proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00040_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, including
`backend_x86_handoff_boundary`, and
`c_testsuite_x86_backend_src_00040_c` now passes. The accepted narrowing was
important during implementation: an earlier broader save/restore variant made
`00040` pass but regressed the canonical short-circuit boundary subset, so the
final slice limits function-level callee-saved preservation to prepared
guard-chain functions that participate in same-module calls. The helper
cleanup also narrowed cleanly: removing the minimal local-slot return call site
kept the exact proof green, while the scalar-load use remains part of the
passing `00040` repair. This Step 2 slice is now reduced to the smallest proven
subset inside the owned file.
