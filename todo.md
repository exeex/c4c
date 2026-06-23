Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe And Close Classification

# Current Packet

## Just Finished

Completed idea 316 Step 5 reprobe/classification after the pointer/select
repairs. Captured BIR, prepared-BIR, RV64 assembly, clang link, and qemu
outcomes for `src/00005.c`, `src/00077.c`, `src/00143.c`, and `src/00144.c`
under `build/rv64_c_testsuite_probe_latest/triage_316_step5/`.

Final candidate status:

- `src/00005.c`: pass. The pointer-to-pointer local frame-address repair holds;
  emit/link/qemu all succeed.
- `src/00077.c`: separate stack-homed fused compare/control-flow residual.
  `foo` emits only through `lw s2, 0(a0)` and then falls through to `main`;
  prepared facts show the first branch condition `%t5` is stack-homed.
- `src/00143.c`: separate stack-homed fused compare/control-flow residual.
  The first loop condition truncates after spilling `%t0`; prepared facts show
  `%t0` and `%t1` are stack-homed before the branch.
- `src/00144.c`: remaining nested select-chain/store-source publication
  residual. Early pointer select publication now emits, but later nested
  select chains still truncate with repeated `mv t0, t0`; prepared store-source
  records show `missing_destination_access` at several later ternary join
  blocks.

## Suggested Next

Hand this classification to the supervisor/plan owner for lifecycle action.
The coherent follow-up split is stack-homed fused compare/control-flow emission
for `src/00077.c` and `src/00143.c`; `src/00144.c` should be a separate nested
select-chain/store-source publication follow-up if the plan owner keeps that
capability in scope.

## Watchouts

- Do not treat Step 5 evidence as a capability repair; this packet only
  reprobed and classified after the landed pointer/select repairs.
- `src/00143.c` has a large later i16 scalarized array/select body, but the
  first bad fact is earlier: stack-homed loop-condition compare/branch
  emission.
- `src/00144.c` is not the same focused pointer-typed local publication case
  repaired in Step 4; its remaining issue is deeper nested select-chain
  publication with missing destination-access facts.

## Proof

Probe artifacts:
`build/rv64_c_testsuite_probe_latest/triage_316_step5/probe_results.tsv` and
`build/rv64_c_testsuite_probe_latest/triage_316_step5/summary.md`.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The build succeeded, but
the backend subset still did not pass because the existing
`backend_riscv_prepared_edge_publication` test failed with "RISC-V prepared
module should emit a register edge move"; `test_after.log` is the proof log.
