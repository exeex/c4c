Status: Active
Source Idea Path: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Function Address Materialization And Local Storage

# Current Packet

## Just Finished

Step 3 RV64 function-address materialization and local function-pointer storage
repair for idea 315.

Completed repair boundary:

- RV64 direct-global address materialization now accepts same-module defined
  function symbols, emitting their labels with `lla`.
- Pointer local stores of direct-global function symbols materialize into the
  prepared pointer home register when one exists, then store that pointer value
  to the local frame slot.
- Prepared register-backed indirect calls now emit `jalr` through the prepared
  indirect-callee register while preserving the existing direct-call path.

Focused coverage changes:

- Converted `riscv64_function_pointer_local_storage.c` from expected-repair to
  positive dump/codegen contracts and added an RV64 qemu runtime case.
- Kept `riscv64_function_pointer_return_chain.c` as expected-repair, but updated
  the codegen contract to show the main-path indirect `jalr` is now emitted and
  the remaining gap is function-pointer return materialization in `choose`.

Probe result:

- `src/00087.c`: emit `0`, clang `0`, qemu `0`, classification `pass`.
- Artifacts:
  `build/rv64_c_testsuite_probe_latest/triage_315_step3/probe_results.tsv`
  and `build/rv64_c_testsuite_probe_latest/triage_315_step3/summary.md`.

## Suggested Next

Proceed to Step 4 by repairing or classifying function-pointer return
materialization for pointer-typed direct function symbols, using
`riscv64_function_pointer_return_chain.c` and `src/00124.c` as the acceptance
bucket.

## Watchouts

- Do not special-case filenames, symbol names, field offsets, or local
  aggregate shapes.
- Do not replace indirect calls with direct calls unless the function-pointer
  semantics are still genuinely preserved.
- The local-storage path is now positive and forbids direct-call substitution
  (`call add_three`) while requiring `jalr`.
- `src/00124.c` remains split: the final indirect call in `main` can now emit
  `jalr`, but `choose` still returns `mv a0, t0` for `return sub` without first
  materializing `sub` into `t0`.
- Same-module function materialization intentionally rejects declarations; do
  not use this path to bypass unsupported external-call diagnostics.
- Keep external empty-stub policy, broad aggregate repair, and unrelated global
  data object work out of scope.

## Proof

Ran delegated proof:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_function_pointer_(local_storage|return_chain)'`
- Emit/link/qemu probe for `src/00087.c` into
  `build/rv64_c_testsuite_probe_latest/triage_315_step3/`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Focused result:

- Build passed.
- Focused Step 3 dump/codegen tests passed: 4/4.
- Added runtime case
  `backend_rv64_runtime_riscv64_function_pointer_local_storage`; it passed in
  the final backend run.
- `src/00087.c` probe passed qemu with exit status `0`.

Results:

- Build passed.
- Delegated backend proof wrote `test_after.log`.
- Backend proof failed with `backend_riscv_prepared_edge_publication` as the
  only failing backend test; the Step 3 function-pointer tests and runtime case
  passed.
