Status: Active
Source Idea Path: ideas/open/376_aarch64_sizeof_loop_bound_stack_home_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Constant Home Publication

# Current Packet

## Just Finished

Step 2 repair refined after supervisor-side backend validation found that the
first implementation preempted established cast and stack-publication tests.
`src/backend/mir/aarch64/codegen/dispatch.cpp` now keeps the same-block integer
constant evaluator, but the new fused-compare branch path is gated to the
missing-publication shape: a named RHS produced by a same-block integer binary
constant expression, backed by a prepared stack home, and not already published
by current block-entry moves. Existing immediate RHS, cast-owned, and
stack-publication-owned fused branch routes retain priority.

Generated `build/c_testsuite_aarch64_backend/src/00205.c.s` no longer has the
first-bad-fact compare pattern. The outer loop header now uses
`ldr w13, [sp, #4]`, `sxtw x9, w13`, `cmp x9, #9`; the inner loop header uses
`ldr w13, [sp]`, `sxtw x9, w13`, `cmp x9, #4`. The previous RHS stack-home
loads from `[sp, #32]` and `[sp, #56]` are gone from the loop-header compare
paths.

## Suggested Next

Supervisor should decide whether Step 2 is acceptance-ready for commit after
the targeted backend regression proof. A coherent next packet, if more work is
desired before commit, is to inspect any remaining historical `00205`
aggregate/global value risks now that the scalar loop-bound first bad fact is
removed.

## Watchouts

The repair is semantic for same-block integer constant expressions and the
specific missing-publication fused-compare RHS shape; it does not hard-code
`00205`, `cases`, or stack offsets. The refinement specifically avoids
preempting `backend_aarch64_branch_control_lowering` cast-owned sign-extension
branches and `backend_aarch64_instruction_dispatch` stack-publication routes.
Broader validation remains a supervisor decision.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00205_c|backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch)$'; } > test_after.log 2>&1`

Result: passed. The build rebuilt `dispatch.cpp` and linked affected targets;
CTest ran `backend_aarch64_branch_control_lowering`,
`backend_aarch64_instruction_dispatch`, and
`c_testsuite_aarch64_backend_src_00205_c`; all three passed. The canonical
proof log is `test_after.log`.
