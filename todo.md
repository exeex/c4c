Status: Active
Source Idea Path: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Timeout Mechanism

# Current Packet

## Just Finished

Step 1: inspected the focused `00205` timeout mechanism after the
sign-extension legality fix.

The accepted timeout baseline is `test_before.log`: focused CTest links the
AArch64 backend tests, `00064` and `00139` pass, and
`c_testsuite_aarch64_backend_src_00205_c` times out after 5.01 seconds.

Generated `build/c_testsuite_aarch64_backend/src/00205.c.s` preserves the
sign-extension legality fact as `sxtw x9, w13`, not `sxtw w9, w13`.

The timeout mechanism is not assembler legality or runner policy. The emitted
`main` enters an unconditional outer-loop back-edge before any output:
`j` is initialized at `[sp, #4]`, `.LBB89_2` loads it, `.LBB89_5` increments it,
stores it back, and branches directly to `.LBB89_2`. The normal exit and inner
case-printing blocks are therefore unreachable. A bounded direct runtime probe
of the existing binary timed out after 1 second with no output, consistent with
spinning before the first `printf`.

The nearby inner-loop lowering is also suspicious once reachable: it computes
the expected bound value `32 >> 3 == 4`, stores it at `[sp, #56]`, but then
compares `x9` with itself after `sxtw x9, w13` (`cmp x9, x9`) instead of
comparing the index against the computed bound.

## Suggested Next

Delegate the next packet to trace and repair general lowering for loop
conditions whose bounds come from constant `sizeof` expressions, starting with
the missing outer `j < sizeof(cases)/sizeof(cases[0])` compare and preserving
the existing legal AArch64 sign-extension width.

## Watchouts

- Do not claim pass-count progress from the timeout residual.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, proof-log policy, or CTest registration.
- Do not regress the idea 303 legality repair.
- Avoid a testcase-shaped branch insertion for `00205`; the evidence points to
  general loop-condition lowering for constant `sizeof` bounds.
- Check both the outer loop and the inner loop: the outer condition is missing,
  while the inner condition appears to compare the sign-extended index against
  itself rather than the computed bound.

## Proof

Observation-only executor packet. No implementation, tests, expectations,
allowlists, unsupported classifications, timeout policy, runner behavior, CTest
registration, `plan.md`, source idea, `test_before.log`, or `test_after.log`
were changed.

Accepted baseline inspected:

`test_before.log`

Narrow non-mutating probes:

`nl -ba build/c_testsuite_aarch64_backend/src/00205.c.s | sed -n '1,240p'`

`timeout 1s build/c_testsuite_aarch64_backend/src/00205.c.bin | head -c 1200`

Runtime probe result: timeout status `124`, no captured output.
