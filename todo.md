Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe And Close Classification

# Current Packet

## Just Finished

Step 5 final reprobe and classification summary for idea 314.

Final candidate statuses:

- `src/00019.c`: emit/link/qemu pass. Step 3 repaired the nested aggregate
  self-pointer/subobject representative.
- `src/00046.c`: emit/link pass, qemu exits `2`. Aggregate stores/reloads are
  emitted, and the remaining failure is a separate select-chain/short-circuit
  runtime lowering residual.
- `src/00140.c`: emit/link pass, qemu exits `132`. Assembly remains truncated
  at `f1` byval aggregate handling, classified as a separate RV64 prepared
  byval aggregate call ABI residual.

Step 5 artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_314_step5/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md`

Closure recommendation:

- Idea 314 appears closure-ready from the executor perspective if its intended
  scope is the aggregate-local subobject repair plus classification of the
  remaining byval/control-flow residuals.
- Separate follow-up ideas are needed for `src/00046.c` select-chain /
  short-circuit runtime lowering and `src/00140.c` RV64 prepared byval
  aggregate call ABI.

## Suggested Next

Hand Step 5 classification to the supervisor/plan owner for lifecycle
decision. Do not close directly from executor role.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- `src/00046.c` is no longer an aggregate-store/reload emission failure; it is
  a select-chain/short-circuit runtime lowering residual.
- `src/00140.c` should not be repaired by only teaching the callee to read
  `%p.f` from a stack-slot home; caller-side aggregate-address/byval payload
  transport is also missing from the simple prepared call emitter.
- Do not fold vararg/floating aggregate ABI behavior into idea 314 closure; use
  a separate byval ABI idea if repairing `src/00140.c`.

## Proof

Ran:

- `cmake --build --preset default -j`
- emit/link/qemu probes for `src/00019.c`, `src/00046.c`, and `src/00140.c`
  into `build/rv64_c_testsuite_probe_latest/triage_314_step5/`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_aggregate_local_(self_pointer_chain|anonymous_union_fields)'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- Probe results:
  - `src/00019.c`: emit `0`, link `0`, qemu `0`, `pass`.
  - `src/00046.c`: emit `0`, link `0`, qemu `2`,
    `separate_select_chain_runtime_failure`.
  - `src/00140.c`: emit `0`, link `0`, qemu `132`,
    `separate_byval_aggregate_call_abi_residual`.
- Focused aggregate-local tests passed `6/6`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing backend
  test.
