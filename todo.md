Status: Active
Source Idea Path: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Materialized Pointer Store Rule

# Current Packet

## Just Finished

Plan Step 2 repaired the AArch64 pointer-addressed `StoreLocal` writeback
boundary without expectation changes. The dispatch path now handles both:

- a selected store whose explicit pointer address is materialized at the same
  instruction, by retargeting the memory address to the materialized register;
- the `00181` shape where the pointer value itself is stack-homed, by
  publishing the stored scalar, reloading the stack-homed pointer, and emitting
  a real store through that address.

Focused backend coverage was added for both shapes in
`backend_aarch64_instruction_dispatch`. Generated `Move` assembly now advances
past the old first bad fact: before `PrintAll`, it reloads `%t47` from
`[sp, #168]` and emits `str w9, [x10]`, then reloads `%t50` from `[sp, #184]`
and emits the zeroing `str w9, [x10]`.

`00181` no longer fails as unchanged subsequent states. Its new first bad fact
under the delegated proof is a 5 second runtime timeout. The live residual is
likely in the adjacent load-side pointer-offset materialization already noted
in Step 1: the same `Move` block still shows suspicious address scaling such
as `mov x9, #4; mul x9, x9, x9` around pointer-derived loads, which is outside
this store-writeback repair unless the supervisor chooses to keep it in the
same source idea.

## Suggested Next

Classify the `00181` timeout from the repaired `Move` assembly and decide
whether the adjacent pointer-derived load/address scaling residual belongs in
Step 3 of this active idea or should split into a separate source idea.

## Watchouts

- Keep the idea 360 starting state correct: `A: 1 2 3 4`, `B/C` zero.
- Do not reintroduce the reviewed unguarded `StoreLocal` fallback; the current
  repair is limited to explicit pointer-addressed stores with focused backend
  coverage.
- Keep `00170`, `00189`, and the named backend contracts stable.
- The store writeback now exists in generated assembly, but `00181` times out;
  avoid treating the timeout as store-regression evidence without first
  checking the adjacent pointer-derived load/address scaling path.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`

Result: build up to date, 6/7 tests passed. The only failing test is
`c_testsuite_aarch64_backend_src_00181_c`, now as a timeout rather than the
old unchanged-state runtime mismatch. `backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_prepare_frame_stack_call_contract`,
`backend_cli_dump_prepared_bir_local_arg_call_contract`,
`c_testsuite_aarch64_backend_src_00170_c`, and
`c_testsuite_aarch64_backend_src_00189_c` pass. `test_after.log` preserves the
delegated proof output. `git diff --check` passes.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
