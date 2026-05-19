Status: Active
Source Idea Path: ideas/open/315_aarch64_large_frame_adjustment_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Frame Adjustment Paths

# Current Packet

## Just Finished

Lifecycle closed idea 314 after Step 4 classified both remaining focused
residuals outside the large stack-offset instruction-spelling owner. It created
two follow-up source ideas:

- `ideas/open/315_aarch64_large_frame_adjustment_materialization.md`
- `ideas/open/316_aarch64_frame_slot_layout_consistency.md`

Idea 315 is now active because `00204.c` has a direct frame-printer
materialization failure:

```text
cannot print AArch64 machine node family=frame opcode=frame_setup:
frame adjustment immediate is outside the plain #imm encoding range 0..4095
```

## Suggested Next

Execute Step 1 of the active plan. Localize the frame setup and teardown
printer/lowering paths for large frame sizes, confirm the `00204.c`
`frame_size=5776` fact, identify existing local backend coverage, and record
the smallest focused proof command before implementation.

## Watchouts

- Do not fold idea 316's `00216.c` frame-size/slot-offset mismatch into this
  owner.
- Do not reopen idea 314's stack-slot load/store or scalar stack-publication
  instruction-spelling paths unless fresh evidence proves the frame adjustment
  residual depends on them.
- Preserve direct frame setup/teardown output for encodable frame sizes.
- No implementation files, tests, proof logs, review artifacts, expectations,
  unsupported classifications, runners, or CTest registration were changed by
  this lifecycle transition.

## Proof

Close-time regression guard for idea 314 used the existing matching focused
`test_before.log` and `test_after.log`.

Strict monotonic mode reported 10/12 before and 10/12 after with zero new
failures but failed only because the pass count did not increase. Non-decreasing
lifecycle close mode passed:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```
