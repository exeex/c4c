Status: Active
Source Idea Path: ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The First Wrong Hanoi Move Boundary

# Current Packet

## Just Finished

Lifecycle switch: idea 363 closed as complete for the prepared
select/condition stale join reload owner, and the later `00181` first-move
runtime mismatch was split to
`ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md`.

## Suggested Next

Plan Step 1 - Localize The First Wrong Hanoi Move Boundary: reproduce the
focused subset, then inspect `00181.c`, semantic BIR, prepared BIR, and
generated AArch64 around the first recursive move, peg arguments, and first
tower store/update. Record the first boundary where expected `C` becomes actual
`B`.

## Watchouts

- Do not repair this by matching `00181`, `Move`, Hanoi tower globals, peg
  letters, output text, block labels, stack offsets, ABI registers, or emitted
  instruction neighborhoods.
- The stale join reloads from idea 363 are gone in live generated `00181`; do
  not reopen that owner unless they reappear.
- Preserve idea 363's publication-scope guard:
  `source_parallel_copy_successor_label == context.control_flow_block->block_label`.
- Preserve idea 362's address-scaling evidence: the index carrier and
  immediate scale must stay in distinct registers for pointer-derived offsets.
- Preserve idea 361's materialized pointer store writeback evidence.
- Preserve idea 360's correct starting-state evidence.

## Proof

Close-time guard for idea 363 used canonical focused logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS, with no new failing tests and no pass-count regression. The
known `00181` failure advanced from segmentation fault to runtime mismatch.
