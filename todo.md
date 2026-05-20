Status: Active
Source Idea Path: ideas/open/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Fixed-Offset Skip Boundary

# Current Packet

## Just Finished

Lifecycle switch activated idea 343 after closing idea 342. No executor packet
has run for this plan yet.

## Suggested Next

Start Step 1 by localizing where consecutive prepared Duff copy offsets become
generated AArch64 offsets that skip every other short-copy slot.

## Watchouts

- Preserve idea 342's repaired latch behavior; do not reopen the duplicate
  decrement path unless fresh evidence shows it returned.
- Do not special-case `00143`, `.LBB` labels, block numbers, stack offsets,
  source lines, or emitted instruction spellings.
- Keep expectation, unsupported, runner, timeout, CTest registration, and
  proof-log policy unchanged.

## Proof

Close-time guard for idea 342 used existing matched focused subset logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Both logs report 5 passed and 1 failed out of 6 with no new
failing tests.
