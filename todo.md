Status: Active
Source Idea Path: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 3 reran the delegated five-test proof and classified the remaining
`00143` runtime residual with prepared-BIR, generated-assembly, and runtime
probes under `/tmp`. The Step 2 focused repair is still proven for the backend
dispatch and machine-printer targets, and the representative still emits the
previously missing case-0/LBB1_27 `ldrh`/`strh` copy sequence.

Fresh representative evidence shows the repair is not complete for all Duff
fallthrough fixed-offset copies. `/tmp/c4c_00143_prepared_bir.txt` has prepared
i16 copy operations in fallthrough blocks 9 through 15, such as block_9
loading `addr %lv.a.0+2` and storing `addr %lv.b.0+2`; the corresponding
generated assembly in `/tmp/c4c_00143_step3.s` for `.LBB1_8` through
`.LBB1_20` only updates `%lv.from`/`%lv.to` stack homes and branches onward,
with no `ldrh`/`strh` data copy in those blocks. Temporary runtime probes also
show the backend binary exits 1 while host clang exits 0, and a probe returning
`n` after the Duff loop exits with backend value 1 versus host value 0.

## Suggested Next

Repair the remaining AArch64 prepared-memory dispatch/emission gap for all
prepared fixed-offset local load/store copies in Duff fallthrough blocks, not
only the case-0 path. Keep `00143` as representative evidence, but prove the
repair with a focused backend fixture that has multiple fallthrough case blocks
with i16 fixed-offset local copies and no testcase-shaped matching.

## Watchouts

- This is fresh contrary evidence to the earlier provisional classification:
  `00143` is still inside idea 341's fallthrough fixed-offset local
  load/store emission family, so this packet does not recommend close or split
  yet.
- There may be a second control-flow/condition residual: the generated
  do-while condition around `.LBB1_29` stores `--n` but branches using another
  `n - 1` compare, and the `/tmp/c4c_00143_probe_n` runtime probe leaves
  backend `n == 1` where host clang leaves `n == 0`. Treat the missing
  fallthrough data copies as the first bad generated-assembly fact because
  prepared BIR already contains the relevant memory operations.
- Temporary evidence paths: `/tmp/c4c_00143_prepared_bir.txt`,
  `/tmp/c4c_00143_step3.s`, `/tmp/c4c_00143_first_mismatch.*`, and
  `/tmp/c4c_00143_probe_*`.

## Proof

Ran the delegated proof:
`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`.
The build succeeded; `backend_aarch64_instruction_dispatch`,
`backend_aarch64_machine_printer`, `00086`, and `00111` passed. `00143` still
fails `[RUNTIME_NONZERO]` with exit 1. The supervisor-selected proof was
sufficient to preserve Step 2's passing focused/backend subset and expose the
remaining representative failure; classification required temporary generated
assembly and runtime probes because `test_after.log` contains no stdout/stderr
from `00143`. Proof log path: `test_after.log`.
