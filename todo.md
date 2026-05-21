Status: Active
Source Idea Path: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Stack-Preserved Pointer Formal Post-Call Handling

# Current Packet

## Just Finished

Plan Step 2 repaired the stack-preserved pointer-formal post-call overwrite in
AArch64 dispatch lowering. `publish_stack_preserved_call_values` now treats an
existing prior stack-preservation record for a value as authoritative and does
not republish the value's stale emitted/home ABI register into the stack slot
again after an intervening call. Added backend dispatch coverage proving a
stack-preserved value initially publishes to its stack home before a clobbering
call, then a later call argument reloads from that stack home instead of
overwriting it from the clobbered incoming register.

## Suggested Next

Classify the remaining `00181` runtime output mismatch as a follow-on residual
outside the now-repaired stack-preserved pointer formal post-call overwrite:
the first visible bad fact is in the printed starting state before recursive
post-call `%p.spare` reuse is observable.

## Watchouts

- Generated `Hanoi` after the repair still performs the correct initial
  publications `str w0, [sp]` and `str x3, [sp, #8]` before the first recursive
  call, but no longer emits the old post-call `str x3, [sp, #8]` between
  `bl Move` and the final `ldr x1, [sp, #8]` / `bl Hanoi` sequence.
- Scalar `%p.n` stack reload behavior remains present: after the intervening
  `Move`, code still reloads `[sp]` and computes `n - 1` before the final
  recursive call.
- Callee-saved pointer homes remain stable for `%p.source`/`%p.dest` through
  `x20`/`x21`.
- The remaining `00181` failure is no longer the old segfault. It is
  `RUNTIME_MISMATCH`, with the first visible bad output in the starting state:
  expected `A: 1 2 3 4`, actual `A: 1 2 0 4`. That appears before the repaired
  recursive post-call `%p.spare` overwrite site is reached.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build was up to date; 6/7 selected tests passed. Passing tests:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c`. The only failure was
`c_testsuite_aarch64_backend_src_00181_c`, now `RUNTIME_MISMATCH` instead of
the prior segfault. Proof log: `test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
