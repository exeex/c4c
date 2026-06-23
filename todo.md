Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Iterate Or Split Remaining Candidate Buckets

# Current Packet

## Just Finished

Completed idea 316 Step 4 evidence re-evaluation after the pointer-to-pointer
and pointer/integer select-chain repairs. Reprobed `src/00005.c`,
`src/00077.c`, `src/00143.c`, and `src/00144.c` with BIR, prepared-BIR, RV64
assembly emission, clang link, and qemu execution where possible. Probe
artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_316_step4/`.

Classification from the reprobe:

- `src/00005.c`: now passes emit, link, and qemu. This appears covered by the
  Step 3 pointer-to-pointer local-address repair.
- `src/00077.c`: emits and links, but qemu exits 139. The first bad emitted
  code is a truncated `foo` after `lw s2, 0(a0)`; prepared BIR shows the first
  compare result `%t5` stack-homed before the fused branch. This looks like a
  separate stack-homed compare/control-flow residual before later pointer
  checks.
- `src/00143.c`: emits and links, but qemu exits 132. Emission truncates in the
  first loop condition after stack-homing `%t0`; prepared BIR has
  `branch_condition for.cond.1` from `slt i32 %t0, 39` with `%t0` and `%t1`
  stack-homed. This also looks like a separate stack-homed compare/control-flow
  residual before later array/select work.
- `src/00144.c`: emits and links, but qemu exits 132. Prepared BIR has
  `%t16 = bir.select ne i32 %t9, 0, ptr 0, 0` with available select-chain and
  store-source records, while assembly truncates after pointer-select no-op
  moves in `tern.end.15`. This remains an in-scope pointer/select residual, but
  specifically pointer-typed select materialization/publication rather than the
  Step 3 I32 select-chain repair.

## Suggested Next

Split or schedule a focused repair for the `src/00144.c` pointer-typed select
materialization/publication residual. Treat `src/00077.c` and `src/00143.c` as
separate stack-homed compare/control-flow work unless the plan owner chooses to
expand the current route.

## Watchouts

- This packet is evidence-only; it does not claim new backend capability beyond
  classification.
- `src/00005.c` is now a passing representative and should not drive more
  pointer-to-pointer repair in this idea.
- `src/00077.c` and `src/00143.c` still need re-evaluation after stack-homed
  compare/control-flow emission is repaired; their later pointer/array bodies
  are not yet the first bad mechanism.
- `src/00144.c` is the remaining in-family pointer/select candidate and should
  not be conflated with the stack-homed compare/control-flow failures.

## Proof

Ran build proof `cmake --build --preset default -j`, then captured BIR,
prepared-BIR, emitted RV64 assembly, clang link, and qemu outcomes for
`src/00005.c`, `src/00077.c`, `src/00143.c`, and `src/00144.c` into
`build/rv64_c_testsuite_probe_latest/triage_316_step4/`. Recorded
`probe_results.tsv` and `summary.md` there.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The build was current
and the backend subset still did not pass because
`backend_riscv_prepared_edge_publication` failed with "RISC-V prepared module
should emit a register edge move"; `test_after.log` is the proof log.
