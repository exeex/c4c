Status: Active
Source Idea Path: ideas/open/303_aarch64_sign_extension_assembler_legality.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Sign-Extension Width Spelling

# Current Packet

## Just Finished

Step 2 repaired `lower_fused_compare_branch_from_emitted_cast(...)` so fused
sign-extension compare branches derive the destination/scratch and compare
operand register views from the widened cast result type while keeping the
extension source operand in the narrow W view.

Changed files:
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
- `todo.md`

Added dispatch coverage for an emitted i32 source feeding a same-block
`SExt i32 -> i64` fused compare branch. The regression expectation requires
`sxtw x9, w13` followed by `cmp x9, #0`.

## Suggested Next

Delegate the next packet against the post-assembly residual for focused
`00205`: the generated assembly is now accepted, but the test binary times out.

## Watchouts

- Keep this owner scoped to AArch64 sign-extension width/spelling legality.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- `build/c_testsuite_aarch64_backend/src/00205.c.s:29` now contains the legal
  spelling `sxtw x9, w13`; do not regress this while investigating the timeout.
- The timeout is a different residual from the accepted `test_before.log`
  assembler failure and was not expanded in this packet.

## Proof

Ran the delegated proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00064|00139|00205)_c'; } > test_after.log 2>&1`

Result in `test_after.log`: build succeeded; `00064` passed; `00139` passed;
`00205` no longer fails assembly on `sxtw w9, w13` and instead times out after
5.01 seconds. The generated assembly at
`build/c_testsuite_aarch64_backend/src/00205.c.s:29` contains `sxtw x9, w13`.
