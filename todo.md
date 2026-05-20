Status: Active
Source Idea Path: ideas/open/333_aarch64_opi_pointer_integer_operation_result_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The OPI Result Publication Owner

# Current Packet

## Just Finished

Lifecycle switched from the parked MOVI zero-extension idea to this OPI
result-publication residual. No implementation work has been performed in
this lifecycle packet.

## Suggested Next

Execute plan Step 1. Start from `tests/c/external/c-testsuite/src/00204.c:476`
and generated `build/c_testsuite_aarch64_backend/src/00204.c.s`: `addip0`
emits `add w0, w0, #0` followed by `mov x0, x13`, and caller `opi` saves
returned `x0` to `x20` but publishes `x19` to `pll`.

## Watchouts

Do not reopen the repaired MOVI BIR immediate cast fold unless new evidence
shows a remaining MOVI mismatch. Keep HFA/byval/stdarg/fixed-formal/local-
value guardrails and `review/326_stdarg_byval_route_review.md` untouched.

## Proof

Lifecycle-only packet. Ran the close-time regression guard for idea 332 on the
available canonical logs:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: failed because the logs do not form a matching close-gate scope after
the appended representative run (`before: passed=6 failed=1 total=7`; `after:
passed=0 failed=1 total=1`). Idea 332 was parked, not archived.
