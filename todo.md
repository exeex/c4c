Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize HFA/Floating First Bad Fact

# Current Packet

## Just Finished

Idea 325 is closed as complete. Commit `5001cecf6 repair AArch64 local value
publication` repaired the Step 2/3 local/value-home publication owner and
added focused local coverage. The remaining `00204.c` failure is classified as
outside that owner: after fixed-size string argument cases,
`fa_hfa11(hfa11)` prints `0.0` instead of `11.1`, followed by corrupted
floating/HFA output and a later segmentation fault.

New active idea: `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`.
Active plan: Step 1, localize the HFA/floating first bad fact.

## Suggested Next

Localize where the expected `11.1` HFA value is lost or misrouted in generated
AArch64 artifacts for `00204.c`. Distinguish HFA call-lane lowering,
aggregate/floating `va_arg` source/progression, register-save-area addressing,
overflow-area addressing, lane materialization, and consumer reads before
editing code.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, aggregate/floating `va_arg` source/progression, frame-size
  coverage, fixed-formal publication, and local/value-home publication.
- Branch publication for the `for.cond.70` fused compare, literal pointer
  provenance for `%t5`/`@.str30`, frame-slot address call operands for
  `%lv.t7.0`, stale mutable pointer-local provenance, later branch exits, and
  `%t15` predecessor/join source publication are fixed and now have focused
  local coverage.
- The remaining runtime representative failure is no longer a timeout. The
  CTest case exits with `RUNTIME_NONZERO`/segmentation fault after printing
  substantial output. The next first bad fact is the first HFA float argument
  output: `fa_hfa11(hfa11)` prints `0.0` instead of `11.1`.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, the format loop, one
  HFA shape, one float literal, one stack slot, one register, one offset, or
  one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Reopen frame/formal publication only if fresh generated output again shows
  uncovered stack references or fixed-formal clobber before publication.
- Reopen local/value-home publication only if fresh generated output again
  shows an unpublished ordinary local, constant, pattern operand, branch
  condition, call operand, or predecessor/join source.

## Proof

Close-time guard for idea 325 used existing matching focused logs:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with 6/7 passing before and after, no new failing tests, no
pass-count decrease, and no new >30s tests. Strict-increase mode does not
accept closure because the representative remains the sole failing CTest case,
but the remaining failure is the newly classified HFA/floating residual now
tracked by idea 326.
