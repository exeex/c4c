Status: Active
Source Idea Path: ideas/open/316_aarch64_frame_slot_layout_consistency.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Current Frame Layout Boundary

# Current Packet

## Just Finished

Lifecycle switched from parked idea 350 to active idea 316 after the unsigned
div/rem repair exposed a frame-layout first bad fact in `00182`.

## Suggested Next

Execute Step 1 from `plan.md`: localize where `00182` local array storage
requirements stop contributing to the emitted AArch64 frame allocation or
stack-slot layout. Record the first bad boundary and narrow proof subset here.

## Watchouts

- Do not special-case `00182`, `print_led`, `MAX_DIGITS`, `buf`, `00216`, one
  stack offset, one function, or one instruction sequence.
- Do not reopen unsigned div/rem producer publication; idea 350 is parked
  unless fresh evidence shows stale div/rem consumers still exist.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.

## Proof

Lifecycle close gate for idea 350 was checked with:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: close rejected. The focused subset stayed at 3/4 with no new failing
tests and no resolved failing tests; the guard failed because pass count did
not strictly increase. Broader backend validation from the supervisor remains:
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 141/141.
