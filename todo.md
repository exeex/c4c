Status: Active
Source Idea Path: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair RV64 Publication Outcome

# Current Packet

## Just Finished

Completed Step 4 RV64 publication outcome verification for Duff/fallthrough
pointer updates.

Reprobed `tests/backend/case/riscv64_duff_fallthrough_pointer_update_producers.c`
and `tests/c/external/c-testsuite/src/00143.c` through RV64 asm emit, clang
link, and qemu under
`build/rv64_c_testsuite_probe_latest/triage_324_step4/`. Both c4cll outputs
passed qemu, and the clang reference for `src/00143.c` also passed.

No RV64 backend repair was needed. The emitted RV64 consumes the repaired
producer facts by loading the current pointer local, adding the halfword stride,
publishing the next pointer, and using the original pointer for the halfword
load/store transfer in later fallthrough blocks.

Promoted the focused Duff fixture beyond dump coverage by adding:

- `backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers`
- `backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers`

The focused dump/codegen/runtime contracts avoid `src/00143.c`, Duff comments,
fixed stack offsets, and fixed candidate values as behavior contracts.

## Suggested Next

Supervisor/plan owner should decide whether idea 324 is closure-ready. Executor
evidence shows the owned candidate now passes and no Step 4 residual remains.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block names, `%t39`,
  `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- Existing unrelated untracked `review/` artifacts were not touched.
- The only broad backend failure remains the existing
  `backend_riscv_prepared_edge_publication` contract.
- Preserve the Step 3 suffix loop/fallthrough-cycle boundary if any follow-up
  work revisits pointer publication; a same-block-only predicate was too broad.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(duff_fallthrough_pointer_update_producers|loop_carried_pointer_postincrement|prepared_local_array_pointer_step)'`

Result: passed, 9/9 tests. The Duff fallthrough fixture now has positive
dump/codegen/runtime coverage, and the idea 323 loop-carried pointer
post-increment plus prepared local-array pointer-step neighbor tests remained
positive.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 287/288 tests
passing (`test_after.log`). The only remaining failure is the existing
`backend_riscv_prepared_edge_publication` contract. The new Duff fallthrough
codegen/runtime tests passed in the broad run.

Probe artifacts:
`build/rv64_c_testsuite_probe_latest/triage_324_step4/probe_results.tsv` and
`build/rv64_c_testsuite_probe_latest/triage_324_step4/summary.md`.
