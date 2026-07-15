# Current Packet

Status: Active
Source Idea Path: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Write the 734 handoff

## Just Finished

- Step 2 — Publish and verify the selected authority tuple is accepted pending
  supervisor commit: the native DirectScalar `LirSwitch.selector_parameter_authority`
  producer, exact-tuple verifier, and positive/malformed `switch_selector_native`
  coverage passed their matching focused proof and regression guard.

## Suggested Next

- Step 3 — Write the 734 handoff: record only the selected authority tuple,
  consumer relation, rejection boundary, and bounded receiver return action.

## Watchouts

- Preserve adjacent dirty 821/822 and binary/`fneg` hunks. The accepted route admits
  only an unchanged current-function native DirectScalar integer parameter as
  `LirSwitch.selector`; do not reuse binary-LHS authority or materialize an add.

## Proof

- `cmake --build --preset default` passed (no work required), then
  `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed (1/1). Matching guard
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
  passed: baseline 0/1 expected abort; after 1/1 pass. Focused proof output:
  `test_after.log`.
