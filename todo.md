Status: Active
Source Idea Path: ideas/open/322_rv64_empty_loop_exit_successor_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Empty Exit Successor Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for idea 322, focused on
`src/00143.c` empty loop-exit successor emission.

Probe artifacts:

- BIR:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/bir/src_00143_c.bir.txt`
- prepared-BIR:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/prepared/src_00143_c.prepared.txt`
- RV64 assembly:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/asm/src_00143_c.s`
- linked binary:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/bin/src_00143_c.bin`
- linked objdump/symbols:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/linked/`
- command logs:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/logs/`
- summary:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/summary.md`
- status table:
  `build/rv64_c_testsuite_probe_latest/triage_322_step1/probe_results.tsv`

Fresh status for `src/00143.c`:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 132 (`SIGILL`)

First bad mechanism:

- Classification:
  `emitted_empty_reachable_loop_exit_successor_body_omitted`.
- BIR and prepared-BIR both keep `for.cond.1` as a conditional branch with
  true successor `block_1` and false successor `block_2`.
- `block_2` is not empty in BIR/prepared-BIR: it materializes the local array
  base pointers into `%lv.from`/`%lv.to`, initializes `%lv.count`, computes
  `%t21`, and begins a `block_2.switch.case.*` cascade.
- RV64 assembly emits the loop false branch as `j .Lmain_block_2`, and it
  defines `.Lmain_block_2:` at the end of the function text with no body,
  branch, epilogue, or return.
- The linked objdump resolves that jump to `0x132c <_IO_stdin_used>`, the next
  section symbol, and qemu traps with `SIGILL`.
- Semantic i16 body emission now precedes the runtime failure: the first loop
  body contains repeated native `lh`/`sh` select-store sequences and branches
  back through `.Lmain_for_latch_1`.

Route distinction:

- Not idea 321: i16 local-array select/store publication is now emitted with
  halfword operations before the failure.
- Not idea 319: the false successor label exists; the failure is omitted
  reachable successor body emission, not a missing label.
- Not idea 320: prepared store-source/select-chain records are available and
  the current failure is after the first loop exits into the omitted successor.

## Suggested Next

Add focused expected-repair backend coverage for a reachable loop false
successor whose BIR/prepared block has a real body after the first loop. The
test should prove the successor body is emitted and reachable, not just that
the label exists.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_2`, `_IO_stdin_used`, fixed
  block order, linked section layout, or observed instruction addresses.
- Do not reopen idea 321's i16 local-array select/store publication, idea
  319's stack-homed fused compare control flow, or idea 320's nested
  store-source publication unless fresh evidence proves a regression.
- Do not hide qemu `SIGILL` by marking the supported candidate unsupported,
  skipping runtime proof, or adding an unreachable trap instead of a valid
  return/epilogue path.
- The focused Step 2 test should avoid requiring switch lowering unless the
  smallest reproducible successor-body omission needs it; the owned boundary is
  reachable successor block traversal/emission after loop exit.

## Proof

Probe/build command ran before artifact capture:
`cmake --build --preset default -j`.

Delegated proof to run:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero only because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
current packet is evidence-only and changed no implementation or tests.
