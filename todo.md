Status: Active
Source Idea Path: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe And Close Classification

# Current Packet

## Just Finished

Step 5 final function-pointer candidate reprobe and classification for idea
315.

Final candidate statuses:

- `src/00087.c`: emit `0`, clang `0`, qemu `0`, classification `pass`.
- `src/00124.c`: emit `0`, clang `0`, qemu `0`, classification `pass`.

Final classification:

- `src/00087.c` is covered by same-module function-address materialization for
  local function-pointer storage and register-backed `jalr` indirect-call
  emission.
- `src/00124.c` is covered by same-module function-pointer return
  materialization, null function-pointer return emission, explicit same-module
  symbol lookup, and chained `jalr` indirect-call use.
- Focused backend coverage now includes positive dump, codegen-route, and RV64
  runtime tests for both local-storage and return-chain function-pointer paths.

Final artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_315_step5/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_315_step5/summary.md`

Idea 315 appears closure-ready from the executor evidence, subject to
supervisor-side review/acceptance and the unrelated backend proof caveat noted
below.

## Suggested Next

Ask the plan owner/supervisor to review closure for idea 315. No additional
executor repair packet is suggested for this function-pointer candidate bucket.

## Watchouts

- Do not special-case filenames, symbol names, field offsets, or local
  aggregate shapes.
- Do not replace indirect calls with direct calls unless the function-pointer
  semantics are still genuinely preserved.
- Local-storage and return-chain focused paths are now positive and forbid
  direct-call substitution where relevant while requiring `jalr`.
- The full backend proof still fails only on
  `backend_riscv_prepared_edge_publication`, which predates this evidence
  packet and is outside Step 5 ownership.
- Same-module function materialization intentionally rejects declarations; do
  not use this path to bypass unsupported external-call diagnostics.
- Keep external empty-stub policy, broad aggregate repair, and unrelated global
  data object work out of scope.

## Proof

Ran delegated proof:

- `cmake --build --preset default -j`
- Emit/link/qemu probes for `src/00087.c` and `src/00124.c` into
  `build/rv64_c_testsuite_probe_latest/triage_315_step5/`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_function_pointer_(local_storage|return_chain)'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Focused result:

- Build passed.
- Final probes passed: `src/00087.c` and `src/00124.c` both emitted, linked,
  and ran under qemu with exit status `0`.
- Focused function-pointer dump/codegen/runtime tests passed: 6/6.

Results:

- Build passed.
- Delegated backend proof wrote `test_after.log`.
- Backend proof failed with `backend_riscv_prepared_edge_publication` as the
  only failing backend test; all Step 5 function-pointer probes and focused
  tests passed.
