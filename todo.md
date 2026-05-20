Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Later Stdarg Field Mismatch

# Current Packet

## Just Finished

Lifecycle switched from idea 328 to idea 331 after commit `941d0c1cb`
completed the fixed non-HFA byval placement repair. The old `fa1(s8..s13)`
caller/callee rounded-slot mismatch is gone: the byval `Arguments:` and
`Return values:` sections now match through the repaired fixed aggregate case.

Close for idea 328 was rejected because the available canonical focused logs
show no regressions but no strict pass-count increase:
`passed=6 failed=1 total=7` before and after. The idea remains open but
inactive with a handoff note.

Current first bad fact for this active plan: `c_testsuite_aarch64_backend_src_00204_c`
still fails in the later `stdarg:` block. The first visible actual line is
`ABCDEFGHI ABCDEFGHI ABCDEFGHI stdarg:` where the expected output has six
`ABCDEFGHI` fields before `stdarg:`.

## Suggested Next

Localize the stdarg field mismatch from generated `myprintf`: trace the
expected six `ABCDEFGHI` fields through format traversal, `va_list` cursor
state, selected aggregate bytes, destination buffers, and the observing call
operands. Record the first generated-code divergence and the narrowest proof
command before editing implementation files.

## Watchouts

Do not reopen fixed byval rounded-slot placement, outgoing stack argument
base, HFA/floating publication, non-HFA aggregate materialization, or
post-`va_arg` call setup without generated-code evidence that the first bad
fact moved back to that owner.

## Proof

Supervisor validation before the handoff commit passed
`ctest --test-dir build -j --output-on-failure -R '^backend_'` at 100%.

Existing focused close-gate logs are non-regressive but not closure-accepted
under the strict guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
reported `passed=6 failed=1 total=7` before and after, `delta=0`, and
`result: FAIL` because the passed count did not strictly increase.
