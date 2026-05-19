Status: Active
Source Idea Path: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Or Split The Semantic Owner

# Current Packet

## Just Finished

Step 2: repaired the focused generated-code timeout mechanism in AArch64
fused `sext` compare-branch lowering.

Changed `src/backend/mir/aarch64/codegen/dispatch.cpp` so the fused
sign-extension compare branch can consume same-block constant integer division
producers as encodable compare immediates. This handles loop bounds such as
`sizeof(cases) / sizeof(cases[0])` without requiring the bound to have already
been materialized in a register. The same support path now treats those
constant-bound producer instructions as branch-support instructions, avoiding
an unsupported scalar-lowering diagnostic for the producer consumed only by the
terminator.

Also changed the helper to avoid selecting a sign-extension scratch register
that aliases the other compare operand, removing the classified inner-loop
`cmp x9, x9` self-compare.

Added unit coverage in
`tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp` for an
i32-to-i64 sign-extension fused branch comparing against a same-block
`504 / 56` constant bound. The unit proof passed:
`cmake --build --preset default --target backend_aarch64_branch_control_lowering_test && ./build/tests/backend/mir/backend_aarch64_branch_control_lowering_test`.

Generated `build/c_testsuite_aarch64_backend/src/00205.c.s` now emits
conditional compares for both loop headers and preserves the legal
sign-extension form:
`sxtw x9, w13; cmp x9, #9` for the outer loop and
`sxtw x9, w13; cmp x9, #4` for the inner loop.

## Suggested Next

Delegate the next packet to investigate the now-reachable `00205` output
mismatch. The generated code reads case fields from stack offsets such as
`[sp, #632]`, `[sp, #1064]`, and `[sp, #1496]` while the prologue reserves only
`sub sp, sp, #48`, so the remaining failure appears to be stack-frame/value
materialization rather than the repaired branch/compare timeout path.

## Watchouts

- The delegated proof no longer times out, but it is still red because `00205`
  now runs and fails output comparison.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, proof-log policy, or CTest registration.
- Do not regress the idea 303 legality repair.
- The branch repair is semantic for same-block constant integer division bounds
  and scratch aliasing; do not broaden it into arbitrary scalar constant
  folding without a separate owner.

## Proof

Supervisor-selected proof command was run exactly and wrote `test_after.log`:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00064|00139|00205)_c'; } > test_after.log 2>&1`

Result: command exited nonzero because `00205` failed output comparison, not
because of the previous timeout. `00064` and `00139` passed. `00205` completed
in about 0.05 seconds and failed with garbage case-field output, consistent
with a separate now-reachable value-materialization/frame issue.
