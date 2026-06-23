Status: Active
Source Idea Path: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Function Pointer Return And Indirect Call Emission

# Current Packet

## Just Finished

Step 4 RV64 function-pointer return materialization and chained indirect-call
repair for idea 315.

Completed repair boundary:

- RV64 pointer returns now materialize same-module function symbols into `a0`
  with `lla` before the return epilogue.
- RV64 pointer returns now emit null function-pointer returns as `li a0, 0`.
- Same-module function address labels now use the actual emitted function label
  spelling (`function.name`) and prefer the explicit prepared symbol spelling
  during lookup, avoiding mismaterialization when link-name metadata disagrees.
- Chained indirect-call use remains indirect: calls through function-pointer
  values continue to emit `jalr`.

Focused coverage changes:

- Converted `riscv64_function_pointer_return_chain.c` from expected-repair to
  positive dump/codegen contracts and added an RV64 qemu runtime case.
- Kept the local-storage function-pointer focused tests positive.

Probe result:

- `src/00124.c`: emit `0`, clang `0`, qemu `0`, classification `pass`.
- Artifacts:
  `build/rv64_c_testsuite_probe_latest/triage_315_step4/probe_results.tsv`
  and `build/rv64_c_testsuite_probe_latest/triage_315_step4/summary.md`.

## Suggested Next

Proceed to Step 5 by reproving the idea 315 candidate bucket and summarizing
whether the function-pointer local/return/indirect-call flow appears
closure-ready or still has separate residuals.

## Watchouts

- Do not special-case filenames, symbol names, field offsets, or local
  aggregate shapes.
- Do not replace indirect calls with direct calls unless the function-pointer
  semantics are still genuinely preserved.
- Local-storage and return-chain focused paths are now positive and forbid
  direct-call substitution where relevant while requiring `jalr`.
- `src/00124.c` passes qemu after fixing same-module function label selection
  for address materialization; watch for other cases that expose stale or
  mismatched link-name metadata.
- Same-module function materialization intentionally rejects declarations; do
  not use this path to bypass unsupported external-call diagnostics.
- Keep external empty-stub policy, broad aggregate repair, and unrelated global
  data object work out of scope.

## Proof

Ran delegated proof:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_function_pointer_(local_storage|return_chain)'`
- Emit/link/qemu probe for `src/00124.c` into
  `build/rv64_c_testsuite_probe_latest/triage_315_step4/`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Focused result:

- Build passed.
- Focused Step 4 dump/codegen/runtime tests passed: 6/6.
- Added runtime case
  `backend_rv64_runtime_riscv64_function_pointer_return_chain`; it passed in
  the final backend run.
- `src/00124.c` probe passed qemu with exit status `0`.

Results:

- Build passed.
- Delegated backend proof wrote `test_after.log`.
- Backend proof failed with `backend_riscv_prepared_edge_publication` as the
  only failing backend test; the Step 4 function-pointer tests and runtime
  cases passed.
