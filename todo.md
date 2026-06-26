Status: Active
Source Idea Path: ideas/open/393_rv64_variadic_aggregate_va_arg_cursor_stride.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Aggregate Va Arg Cursor Evidence

# Current Packet

## Just Finished

Lifecycle disposition after idea 387 Step 5 closed the same-module sret owner
and split the later runtime abort into idea 393. The old representative now
advances past unsupported local-memory and same-module memory_return/sret
object-emission gates, compiles, links, and enters callee `f`. The remaining
abort is callee-side RV64 aggregate `va_arg` cursor stride/layout: after the
first aggregate payload read, c4c advances the cursor by 4 bytes instead of
the next 8-byte variadic GPR save-area slot.

## Suggested Next

Execute Step 1 from `plan.md`: capture current prepared/BIR/object evidence
for the aggregate `va_arg` cursor and payload layout in
`tests/c/external/gcc_torture/src/920908-1.c`, then record the exact stride
fact gap in `todo.md`.

## Watchouts

- Do not reopen idea 387 without contradictory evidence; Step 5 showed sret
  call emission advanced to the later runtime owner.
- Do not rewrite caller publication while qemu still shows `a2=10` and
  `a3=20` at callee entry.
- Keep aggregate object size separate from ABI variadic GPR save-area slot
  stride.
- Do not hard-code `920908-1.c`, callee `f`, registers, stack offsets, or the
  abort branch.

## Proof

Lifecycle-only close/split/switch. The plan owner reran the close-gate
regression comparison using the accepted current backend baseline
`test_before.log` as both inputs because `test_after.log` now contains the
todo-only Step 5 representative evidence, not a CTest summary:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed`.

Result: PASS, 326/326 before and 326/326 after, no new failures.
