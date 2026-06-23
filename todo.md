Status: Active
Source Idea Path: ideas/open/314_rv64_aggregate_local_subobject_and_byval_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Nested Aggregate Subobject Flow

# Current Packet

## Just Finished

Step 3 RV64 prepared aggregate-local repair for idea 314.

Implemented repair boundary:

- Added RV64 prepared emission support for pointer-value pointer loads, so
  local self-pointer chains can repeatedly load through aggregate pointers and
  reach nested scalar fields.
- Relaxed RV64 prepared frame-slot stack offset resolution only for offset-zero
  4-byte accesses rooted in byte-split fixed one-byte frame slots in the same
  function, covering local aggregate subobjects without naming structs, fields,
  source files, or fixed testcase offsets.

Converted Step 2 expected-repair tests to positive focused contracts:

- `backend_dump_riscv64_aggregate_local_self_pointer_chain`
- `backend_codegen_route_riscv64_aggregate_local_self_pointer_chain`
- `backend_dump_riscv64_aggregate_local_anonymous_union_fields`
- `backend_codegen_route_riscv64_aggregate_local_anonymous_union_fields`
- `backend_rv64_runtime_riscv64_aggregate_local_self_pointer_chain`
- `backend_rv64_runtime_riscv64_aggregate_local_anonymous_union_fields`

Focused acceptance already run:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_aggregate_local_(self_pointer_chain|anonymous_union_fields)'`

Focused results:

- Build passed.
- Focused dump/codegen/runtime subset passed `6/6`.

Step 3 c-testsuite reprobe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_314_step3/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_314_step3/summary.md`

Probe classification:

- `src/00019.c`: emit/link/qemu pass; nested self-pointer aggregate chain is
  repaired for this representative.
- `src/00046.c`: emit/link pass, qemu exits `2`; aggregate field stores and
  reloads now emit, and the remaining failure is classified as a separate
  short-circuit/select-chain runtime mechanism, not the nested subobject
  emission gap repaired here.

## Suggested Next

After Step 3 proof, keep `src/00046.c`'s qemu residual as a separate
control/select-chain decision unless the plan owner chooses to expand the
current route. Keep `src/00140.c` byval aggregate copy/call flow out of this
packet.

## Watchouts

- Do not special-case filenames, fixed offsets, or struct/union names.
- Do not reopen generic local frame-slot address publication unless aggregate
  evidence proves it is still incomplete.
- Keep broad vararg and floating aggregate ABI repair out of scope unless it is
  only being classified as a residual.
- The current RV64 emitter can emit/link truncated aggregate-local functions
  and only fail later under qemu; focused tests should catch missing emission
  before relying on qemu symptoms.
- `src/00140.c` includes byval aggregate copy and same-module call argument
  flow; do not fold broad byval/variadic/floating aggregate ABI work into the
  first nested-subobject repair packet.
- Anonymous-union prepared addressing still reports `proven_out_of_bounds` for
  byte-split 4-byte subobject accesses. Step 3 repairs the RV64 prepared
  emission boundary using exactly contiguous fixed one-byte frame-slot layout
  evidence; a separate preparation-addressing cleanup can normalize those
  verdicts later.

## Proof

Ran:

- `cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_aggregate_local_(self_pointer_chain|anonymous_union_fields)' && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- emit/link/qemu probes for `src/00019.c` and `src/00046.c` into `build/rv64_c_testsuite_probe_latest/triage_314_step3/`

Results:

- Build passed.
- Focused dump/codegen/runtime subset passed `6/6`.
- `src/00019.c` probe passed qemu.
- `src/00046.c` probe emitted and linked but qemu exited `2`, classified as
  `separate_select_chain_runtime_failure` after aggregate stores/reloads were
  confirmed in assembly.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing backend
  test.
