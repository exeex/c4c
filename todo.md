Status: Active
Source Idea Path: ideas/open/323_rv64_loop_carried_pointer_postincrement_publication.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe and Close Classification

# Current Packet

## Just Finished

Completed Step 5 reprobe/classification for
`tests/c/external/c-testsuite/src/00143.c` after the Step 4 loop-carried
pointer consumption repair.

Artifacts are under
`build/rv64_c_testsuite_probe_latest/triage_323_step5/`:
`probe_results.tsv`, `summary.md`, BIR/prepared-BIR dumps, RV64 assembly,
linked c4cll and clang-reference binaries, and command logs/status files.

Focused proof and the candidate assembly confirm the owned loop-carried pointer
post-increment residual is fixed. The focused
`backend_(dump|codegen_route|rv64_runtime)_riscv64_loop_carried_pointer_postincrement`
tests pass, and `src/00143.c` Duff `block_6` now emits current-pointer-based
updates: load `%lv.from`/`%lv.to`, compute next pointer with `addi ... 2`,
store the updated pointer locals, and copy the halfword through the old pointer
value.

`src/00143.c` still fails at runtime after emit/link. Probe status:
`dump_bir=0`, `dump_prepared=0`, `emit=0`, `clang_link=0`,
`qemu_c4cll=139`, clang reference `build/qemu=0/0`. The qemu strace records
SIGSEGV at `0x0000000500000002`.

The first remaining residual is a separate Duff fallthrough pointer-update
publication route, not the Step 4 RV64 consumption path. The first bad block is
`block_9`, reached by `count % 8 == 7`. BIR stores `%t39` and `%t42` into
`%lv.from`/`%lv.to`, but there is no `bir.add ptr` producer for those next
pointer values:

```text
block_9:
  %t38 = bir.load_local ptr %lv.from
  bir.store_local %lv.from, ptr %t39
  %t40 = bir.load_local i16 %t40.addr, addr %t38
  %t41 = bir.load_local ptr %lv.to
  bir.store_local %lv.to, ptr %t42
  bir.store_local %t41.store.addr, i16 %t40, addr %t41
```

Prepared-BIR gives `%t39` and `%t42` stack homes (`stack216` and `stack224`)
with `source_producer=unknown`; emitted RV64 then loads those uninitialized
homes and publishes them as pointer locals before the later crash.

## Suggested Next

Have the plan owner/supervisor decide whether idea 323 is closure-ready for the
focused loop-carried pointer post-increment repair, with a follow-up idea for
Duff fallthrough pointer-update publication if `src/00143.c` remains a target.

## Watchouts

- Do not claim `src/00143.c` passes; it emits and links but still segfaults.
- Do not special-case `src/00143.c`, Duff's-device block layout, block names,
  `%t39`/`%t42`, fixed stack offsets, or fixed array sizes.
- A follow-up repair should create semantic/prepared next-pointer producer facts
  for Duff fallthrough copy blocks rather than teaching RV64 to guess from
  uninitialized stack homes.

## Proof

Candidate reprobe:
`tests/c/external/c-testsuite/src/00143.c` was run through BIR dump,
prepared-BIR dump, RV64 emit, clang link, qemu for c4cll output, and clang
reference build/qemu. Results are recorded in
`build/rv64_c_testsuite_probe_latest/triage_323_step5/probe_results.tsv` and
`summary.md`.

Result: c4cll emit/link succeeded; qemu exits 139. Clang reference exits 0.
Classification:
`separate-duff-fallthrough-pointer-update-publication-residual`.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 284/285 tests
passing (`test_after.log`). The only remaining failure is the existing
`backend_riscv_prepared_edge_publication` contract. The loop-carried
dump/codegen/runtime tests passed in the broad run.
