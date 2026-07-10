Status: Active
Source Idea Path: ideas/open/657_rv64_loop_2e_indirect_store_writeback_runtime.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove Backend Regression Safety

# Current Packet

## Just Finished

Step 2 of `plan.md` proved backend regression safety against the accepted
full-suite baseline context. The delegated proof command was run exactly and
preserved output in `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`.

The build was up to date and CTest returned nonzero for the accepted baseline
red rows. `test_before.log` and `test_after.log` both report `99% tests
passed, 11 tests failed out of 3397`, and the failed test IDs/names match
exactly. There are no new failures for idea 657.

Remaining red rows are accepted unrelated baseline state:
`backend_dump_riscv64_stack_passed_parameter_home_publication`,
`backend_dump_riscv64_scalar_compare_frame_slot_destination`,
`backend_dump_riscv64_prepared_fused_compare_call_result_predicate`,
`backend_dump_riscv64_byval_aggregate_fixed_call`,
`backend_dump_riscv64_byval_preserved_pointer_args`,
`backend_dump_riscv64_function_pointer_return_chain`,
`backend_riscv_object_emission`,
`backend_aarch64_instruction_dispatch`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
`llvm_gcc_c_torture_src_20040709_2_c`, and
`llvm_gcc_c_torture_src_20040709_3_c`.

## Suggested Next

Delegate Step 3 closure-readiness decision. The Step 3 packet should summarize
that Step 1 refreshed representative `loop-2e.c` RV64 object-runtime evidence
successfully and Step 2 found no new failures against the accepted full-suite
baseline, then request lifecycle close review unless the supervisor sees a
separate blocker.

## Watchouts

- The full-suite proof is intentionally red only because the accepted baseline
  is red; do not route the 11 matching rows through idea 657.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.
- Keep stack-destination fan-in, byval, object-emission, AArch64, CLI, and
  LLVM torture work out of this plan unless fresh evidence proves they became
  idea 657 blockers.
- Preserve the Step 1 explicit prepared `base=pointer_value` access contract and
  do not reopen the completed `%t23` route without new regression evidence.

## Proof

Completed with no new failures against accepted baseline:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`.

Proof log: `test_after.log`.
Baseline log: `test_before.log`.
