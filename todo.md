Status: Active
Source Idea Path: ideas/open/322_rv64_empty_loop_exit_successor_emission.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reprobe Candidate and Classify Residuals

# Current Packet

## Just Finished

Completed Step 4 reprobe for idea 322, focused on
`tests/c/external/c-testsuite/src/00143.c` after the Step 3 empty
loop-exit successor repair.

Probe artifacts:

- BIR:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/bir/src_00143_c.bir.txt`
- prepared-BIR:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/prepared/src_00143_c.prepared.txt`
- RV64 assembly:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/asm/src_00143_c.s`
- linked c4cll binary:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/bin/src_00143_c.bin`
- clang reference binary:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/bin/src_00143_c.clang_ref.bin`
- linked objdump/symbols:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/linked/`
- command logs:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/logs/`
- summary:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/summary.md`
- status table:
  `build/rv64_c_testsuite_probe_latest/triage_322_step4/probe_results.tsv`

Fresh status for `src/00143.c`:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang assembler/link for c4cll output: 0
- qemu for c4cll output: 1
- qemu for clang reference binary: 0

Classification:

- The old idea 322 failure is fixed. RV64 now emits `.Lmain_block_2`, the
  Duff's-device switch/fallthrough body blocks, the later verification loop
  blocks, and valid return blocks. The linked binary no longer falls through
  into `_IO_stdin_used`; qemu exits normally with status 1 rather than
  trapping with `SIGILL`.
- First remaining residual:
  `loop_carried_pointer_postincrement_residual`.
- Emitted-code evidence: the Duff's-device copy blocks repeatedly store fixed
  stack addresses back into `%lv.from`/`%lv.to` homes, for example
  `addi t1, sp, 2; sd t1, 160(sp)` and
  `addi t1, sp, 80; sd t1, 168(sp)` in `.Lmain_block_6`, then similar fixed
  offsets in following fallthrough blocks.
- Prepared-BIR evidence matches that emitted shape, for example
  `%t29 = bir.add ptr %lv.a.0, 2; bir.store_local %lv.from, ptr %t29`.
  Repeated Duff's-device loop iterations therefore rematerialize pointer locals
  from fixed array-base offsets instead of advancing from the current
  loop-carried `from`/`to` values.
- Scope: this residual is outside idea 322's empty loop-exit successor emission
  route and should split as a loop-carried pointer post-increment
  publication/lowering issue if pursued.

## Suggested Next

Plan-owner/supervisor decision: idea 322 appears closure-ready for the owned
empty successor emission route, with the remaining `src/00143.c` wrong-result
classified as a separate loop-carried pointer post-increment residual.

## Watchouts

- Do not reopen idea 322 by treating qemu exit 1 as another empty successor
  fallthrough; the emitted and linked evidence shows the successor body and
  return paths are present.
- Do not special-case Duff's device, `src/00143.c`, stack offsets, block names,
  `_IO_stdin_used`, or section layout in a follow-up.
- The next residual is about preserving loop-carried pointer post-increment
  values across fallthrough/repeated loop iterations.

## Proof

Build/probe command ran before artifact capture:
`cmake --build --preset default -j`.

Candidate reprobe ran through BIR dump, prepared-BIR dump, RV64 asm emit,
clang assembler/link, qemu for the c4cll binary, and qemu for a clang reference
binary.

Delegated proof to run:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero only because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
canonical proof log is `test_after.log`.
