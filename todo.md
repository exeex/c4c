Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Residual Candidate Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for `src/00005.c`, `src/00077.c`,
`src/00143.c`, and `src/00144.c`. Captured BIR, prepared BIR, emitted RV64
assembly, clang link results, qemu results, and classifications under
`build/rv64_c_testsuite_probe_latest/triage_316_step1/`.

## Suggested Next

Add focused expected-repair backend coverage for the first coherent repair
boundary: local pointer-to-pointer frame-address materialization (`&p` stored
through `int **`) and pointer/integer select-chain materialization/publication.

## Watchouts

- Do not special-case candidate filenames, observed stack offsets, or source
  expression shapes.
- `src/00005.c` shows a local frame-address materialization bug: prepared BIR
  records `&p` at stack offset 8, but emitted assembly stores `sp+16` into
  `pp`, pointing it at the wrong local slot.
- `src/00077.c` reaches pointer-parameter array load, then truncates before a
  stack-homed `ne`/branch result; it does not yet classify later local-array
  pointer assignment.
- `src/00143.c` truncates in the first scalar loop compare before the
  Duff-device array/switch body; split or plan-review that scalar
  compare/control-flow residual before treating it as pointer/array evidence.
- `src/00144.c` reaches pointer/integer select materialization and then
  truncates after no-op self moves at `.Lmain_tern_end_15`.
- Keep aggregate/byval and function-pointer repair out of this route.

## Proof

Ran `cmake --build --preset default -j`, all delegated per-candidate BIR,
prepared-BIR, asm emit, clang link, and qemu probes, then
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.
The build and probes completed. The backend proof did not pass:
`backend_riscv_prepared_edge_publication` failed with "RISC-V prepared module
should emit a register edge move"; `test_after.log` is the proof log.
