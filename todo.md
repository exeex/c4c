Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Remaining Duff Fallthrough Copy Emission

# Current Packet

## Just Finished

Step 4 repaired the remaining Duff fallthrough fixed-offset local copy emission
gap. Dispatch now selects the prepared memory-access index when the retained
BIR index either has no prepared access or has a prepared access whose load/store
value identity belongs to a different memory instruction, including memory ops
at retained instruction index 0. Memory validation also accepts prepared
frame-slot authority for structured fixed-offset local addresses, where the
prepared access has already resolved the local address to the concrete frame
slot.

Focused backend coverage now has multiple fallthrough-style fixed-offset i16
local copy blocks and fails if only the original padded branch-block copy path
emits. Regenerated `/tmp/c4c_00143_step4.s` no longer has the prior empty
fallthrough copy blocks: `.LBB1_8` through `.LBB1_20` now contain `ldrh`/`strh`
data movement in addition to the pointer-local stack-home updates.

## Suggested Next

Classify or repair the now-exposed `00143` control-flow residual around the
Duff do-while latch. The generated `.LBB1_29` stores `--n`, then reloads `n`,
subtracts one again, and branches on that second `n - 1` compare, so the backend
terminates the loop one iteration early.

## Watchouts

- The remaining `00143` failure is no longer the fixed-offset fallthrough copy
  emission gap from idea 341 Step 4; the first bad generated-assembly fact has
  moved to the latch condition.
- The memory validation repair intentionally relies on prepared access identity
  plus prepared frame-slot authority; it does not match `00143`, labels, block
  numbers, local names, source lines, or emitted instruction strings.
- Temporary evidence paths from this packet: `/tmp/c4c_00143_step4.s` and
  `/tmp/c4c_00143_step4_prepared_bir.txt`.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`.
The build succeeded; `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, `00086`, and `00111` passed. `00143` still
fails `[RUNTIME_NONZERO]` with exit 1. The supervisor-selected proof was
sufficient for this packet: it proves the focused backend coverage and
non-`00143` AArch64 subset did not regress, while the regenerated `00143`
assembly proves the prior fallthrough copy omission is repaired and exposes the
separate latch-condition residual. Proof log path: `test_after.log`.
