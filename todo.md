# Execution State

Status: Active
Source Idea Path: ideas/open/63_x86_backend_runtime_correctness_regressions.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The First Runtime Seam
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 2 packet repaired the remaining `00130` runtime wrong-result seam by
fixing prepared direct-frame-slot selection for overlapping dotted local-slot
slices in `src/backend/prealloc/stack_layout.cpp`. The bad `*q` path was not a
pointer-register bug after all: semantic BIR had already canonicalized the
load to `addr %lv.arr.4+3`, but prepared frame-slot lookup kept treating
overlapping leaves like `arr.4` and `arr.7` as unrelated homes, so x86 read
`[rsp + 3]` instead of the same byte stored at `[rsp + 24]`. The accepted fix
now resolves dotted local-slot accesses onto the earliest covering slice for
that aggregate root and carries the byte-offset adjustment into the prepared
memory access, which makes the overlapping `00130` loads/stores share one real
stack home again.

## Suggested Next

Move to Step 3 proof/closure work for idea 63. The known owned runtime cases
now appear green in the current tree (`00040`, `00086`, `00130`), so the next
packet should verify the runtime-owned set as a group and then let the
supervisor decide whether plan-owner lifecycle work is needed to close or
retire the active runbook.

## Watchouts

- Keep runtime ownership on wrong-code/correctness; do not reclassify these
  cases back into unsupported-capability buckets now that all three known
  runtime cases run.
- The real seam was prepared stack-layout lookup for overlapping dotted local
  slots, not x86 pointer-register state. Do not “fix” this by adding a named
  `00130` renderer shortcut or by teaching x86 to special-case one aggregate
  spelling.
- The new lookup intentionally prefers the earliest covering dotted slice for a
  shared aggregate root so accesses like `arr.4+3` and direct `arr.7` stores
  land on the same underlying frame home. Nearby aggregate-leaf runtime cases
  may depend on that normalization too.
- The emitted `00130` asm still contains dead pointer-home loads for `p` and
  `q` before the folded byte checks. That is acceptable for this correctness
  packet, but any later cleanup should treat it as a separate optimization
  initiative, not part of the runtime fix.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_.*|c_testsuite_x86_backend_src_00130_c)$' | tee test_after.log`.
Current result: all `74` `backend_*` tests pass, including
`backend_x86_handoff_boundary`, and
`c_testsuite_x86_backend_src_00130_c` now passes. The repaired prepared asm now
loads the `*q` byte from `[rsp + 24]`, matching the earlier `arr[1][3] = 2`
store instead of the old wrong `[rsp + 3]` frame location. A focused pre-check
before implementation also showed `c_testsuite_x86_backend_src_00086_c`
already passing in the current tree, so the known idea-63 runtime set appears
green after this Step 2 slice.
