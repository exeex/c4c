Status: Active
Source Idea Path: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Duff Fallthrough Producer Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence normalization for the missing Duff fallthrough
pointer update producer route.

Fresh artifacts for `tests/c/external/c-testsuite/src/00143.c` are under
`build/rv64_c_testsuite_probe_latest/triage_324_step1/`: `probe_results.tsv`,
`summary.md`, `evidence_excerpt.txt`, BIR/prepared-BIR dumps, emitted RV64
assembly, linked c4cll and clang-reference binaries, and command logs/status
files.

Probe status: `dump_bir=0`, `dump_prepared=0`, `emit=0`, `clang_link=0`,
`qemu_c4cll=139`, clang reference `build/qemu=0/0`. The c4cll binary segfaults
at `0x0000000500000002`.

The route is distinct from idea 323. The repaired idea 323 path still works in
`block_6`: BIR has explicit `%t29 = bir.add ptr %t28, 2` and
`%t32 = bir.add ptr %t31, 2`, prepared store-source records classify them as
`source_producer=binary`, and emitted RV64 computes/stores the next pointer
from the current pointer value before copying the halfword.

The first later bad fallthrough is `block_9`. BIR stores `%t39` and `%t42`
into `%lv.from`/`%lv.to`, but those values have no defining `bir.add ptr`
producer:

```text
block_9:
  %t38 = bir.load_local ptr %lv.from
  bir.store_local %lv.from, ptr %t39
  %t40 = bir.load_local i16 %t40.addr, addr %t38
  %t41 = bir.load_local ptr %lv.to
  bir.store_local %lv.to, ptr %t42
  bir.store_local %t41.store.addr, i16 %t40, addr %t41
```

Prepared store-source records report `source_producer=unknown` for `%t39` and
`%t42`; emitted RV64 then loads uninitialized stack homes (`216(sp)` and
`224(sp)`) and publishes those as pointer locals. The same pattern continues in
later Duff fallthrough blocks.

## Suggested Next

Add focused expected-repair coverage for Duff-style fallthrough pointer update
publication: repeated copy blocks after a switch/fallthrough entry must publish
next-pointer producer facts analogous to the first repaired loop-carried block.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block names, `%t39`,
  `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- Do not teach RV64 to guess from uninitialized stack homes or synthesize
  pointer updates without prepared semantic authority.
- Do not reopen idea 323's RV64 consumption path while later fallthrough blocks
  still have unknown source producers.
- Keep runtime proof on the supported path; do not weaken qemu coverage or
  mark the candidate unsupported.
- The focused test should prove missing producer facts before RV64 consumption,
  not only the final segfault or a fixed observed address.

## Proof

Candidate reprobe:
`tests/c/external/c-testsuite/src/00143.c` was run through BIR dump,
prepared-BIR dump, RV64 emit, clang link, qemu for c4cll output, and clang
reference build/qemu. Results are recorded in
`build/rv64_c_testsuite_probe_latest/triage_324_step1/probe_results.tsv` and
`summary.md`.

Result: c4cll emit/link succeeded; qemu exits 139. Clang reference exits 0.
Classification: `missing-duff-fallthrough-pointer-update-producers`.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 284/285 tests
passing (`test_after.log`). The only remaining failure is the existing
`backend_riscv_prepared_edge_publication` contract. The idea 323
loop-carried dump/codegen/runtime tests passed in the broad run.
