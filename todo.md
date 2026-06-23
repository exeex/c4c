Status: Active
Source Idea Path: ideas/open/322_rv64_empty_loop_exit_successor_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Empty Loop-Exit Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for idea 322, focused on a
reachable empty loop-exit successor whose body is present in prepared BIR but
omitted by current RV64 emission.

New focused fixture:

- `tests/backend/case/riscv64_empty_loop_exit_successor.c`

New CTest coverage:

- `backend_dump_riscv64_empty_loop_exit_successor`
- `backend_codegen_route_riscv64_empty_loop_exit_successor`

Coverage contract:

- The dump test proves the loop false successor reaches a prepared block with
  real pointer-local setup, switch/fused-compare control flow, pointer-value
  memory access, and a semantic `bir.ret i32`.
- The codegen route test records the current expected-repair RV64 failure:
  current emission branches to a generated block label after native `lh`/`sh`
  body emission, but the output has no valid `li a0`/`ret` epilogue path.
- The fixture is independent of `src/00143.c`, `_IO_stdin_used`, linked
  section layout, observed instruction addresses, and a fixed concrete
  `.Lmain_block_2` spelling.

Previous Step 1 evidence remains the source classification:

- `src/00143.c` BIR/prepared-BIR keep `for.cond.1` false successor `block_2`
  with real successor body work.
- RV64 assembly emits the loop false jump and defines the successor label with
  no body, branch, epilogue, or return, so linked execution falls into the next
  section and qemu reports `SIGILL`.
- i16 `lh`/`sh` body emission and stack-homed fused compare label emission
  remain separate repaired boundaries before this failure.

## Suggested Next

Repair RV64 block traversal/emission so reachable loop-exit successor blocks
with real prepared bodies are emitted before the function text closes, then
convert `riscv64_empty_loop_exit_successor` from expected-repair to positive
codegen and runtime coverage.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_2`, `_IO_stdin_used`, fixed
  block order, linked section layout, or observed instruction addresses.
- Do not reopen idea 321's i16 local-array select/store publication, idea
  319's stack-homed fused compare control flow, or idea 320's nested
  store-source publication unless fresh evidence proves a regression.
- Do not hide qemu `SIGILL` by marking the supported candidate unsupported,
  skipping runtime proof, or adding an unreachable trap instead of a valid
  return/epilogue path.
- The Step 2 fixture uses switch-controlled successor work only because the
  smaller loop-tail cases already emit correctly; the owned boundary remains
  reachable successor block traversal/emission after loop exit.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(empty_loop_exit_successor|i16_local_array_select_store|stack_homed_fused_compare_missing_false_label)'`

Result: passed, 8/8 tests.

Delegated proof to run:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero only because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
canonical proof log is `test_after.log`.
