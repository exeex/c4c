Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize HFA/Floating Caller Argument Publication Residual

# Current Packet

## Just Finished

Lifecycle switched back from idea 331 to idea 326 after commit `77bc43d78`
removed the AArch64 `va_start` overflow cursor first bad fact. Idea 331 is
semantically exhausted, but close was not accepted because the available
canonical logs are identical and the close-time regression guard rejected
them for non-increasing pass count.

## Suggested Next

Execute Step 1 by localizing the caller-side HFA/floating argument
publication residual.

New first bad fact after the `va_start` repair:
`c_testsuite_aarch64_backend_src_00204_c` still fails, but the first runtime
mismatch is now earlier than `stdarg`. In the `Arguments:` section, expected
`11.1` from `fa_hfa11(hfa11)` prints as `0.0`. Generated code in `arg` loads
`hfa11` into `s13` and stores it at `[sp, #784]`, then stores stale `s8` at
`[sp, #788]`, reloads `[sp, #788]` into `s13`, moves that into `s0`, and calls
`fa_hfa11`. The callee `fa_hfa11` consumes `s0` normally, so the first bad
fact appears to be caller HFA float argument publication/loading, not the
repaired `va_start` overflow cursor or stdarg cursor/format path.

## Watchouts

Do not reopen stdarg cursor/format or `va_start` overflow cursor
initialization without direct generated-code evidence that the first bad fact
moved back to that owner.

Do not reopen HFA/floating return publication, sret `x8`, large byval indirect
pointer transport, byval aggregate register-lane allocation, fragmented byval
lane publication, non-HFA aggregate `va_arg` materialization, fixed-formal
entry publication, local/value-home publication, frame/formal publication, the
scalar separator call-argument repair, or the byval overflow stack publication
repair without direct generated-code evidence that the first bad fact moved
back to that owner.

Do not special-case `00204.c`, `arg`, `fa_hfa11`, `hfa11`, one HFA shape, one
float value, one stack offset, one register, or one emitted instruction
sequence.

## Proof

Close-time guard check for idea 331:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`
reported `passed=4 failed=1 total=5` for both logs and rejected closure
because the passed count did not strictly increase. No proof logs were
modified.
