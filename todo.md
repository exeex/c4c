Status: Active
Source Idea Path: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Repair Post-Call Scalar Formal Reload And Prove Focused External Progress

# Current Packet

## Just Finished

Steps 2 and 3 repaired the scalar formal preserved-home/post-call reuse
boundary for `00181` and then repaired the backend-wide smoke regression from
that slice.

AArch64 dispatch now emits prepared `BeforeInstruction` value moves for binary
consumers only when the lowered move is a prepared frame-slot reload. It records
the loaded source value in the destination register for the following scalar
lowering. Stack-preserved values are also populated from their pre-call register
home before the first preserving call, while later same-block consumers prefer
the latest prior stack-preserved home over the stale ABI register home.

Focused backend coverage now proves a stack-preserved scalar formal consumed
after an intervening clobbering call is reloaded from its stack home before the
post-call scalar operation. Regenerated `00181` AArch64 no longer computes the
second recursive `n - 1` from clobbered `w0`; the path now stores incoming
`%p.n` to `stack+0` before the first recursive call and reloads it with
`ldr x13, [sp]` before the second decrement.

The backend smoke regression was caused by the first repair emitting ordinary
register-backed binary consumer moves such as `mov x13, x0` before
`add w13, w13, #3`. The new gate keeps those ordinary register-backed scalar
operations on their direct operands, so
`backend_cli_aarch64_asm_external_return_add_smoke` again passes without
weakening its expectation.

## Suggested Next

Ask the supervisor/plan owner whether idea 358 is complete for scalar formal
post-call preservation and whether the remaining `00181` crash should split to
a stack-preserved pointer formal owner.

## Watchouts

- `00181` advanced past the scalar `%p.n`/`w0` first bad fact but still exits
  with `RUNTIME_NONZERO`.
- New first bad fact: a later recursive path still overwrites the stack home
  for stack-preserved pointer formal `%p.spare` from clobbered `x3` after a
  call, then reloads that bad stack slot for a later recursive call. GDB
  confirms the remaining crash is in `Move` with `source`/`x0 = 0`.
- This residual is not the scalar formal `%p.n` post-call preservation
  boundary repaired here.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_cli_aarch64_asm_external_return_add_smoke|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: 7/8 passed. Backend contracts passed, the smoke regression passes,
`00170` and `00189` passed, and
`c_testsuite_aarch64_backend_src_00181_c` remains the single
`RUNTIME_NONZERO` segmentation-fault residual after advancing past the scalar
formal `%p.n` clobbered-`w0` boundary.

Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
