Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Cover Aggregate And Function-Pointer Local Flow

# Current Packet

## Just Finished

Step 4 implementation packet repaired the remaining `src/00130.c` local array
element access tail.

Fresh repro under
`build/rv64_c_testsuite_probe_latest/triage_312_step4_00130/` showed `00130`
emitted and linked but qemu exited 132. The emitted assembly stopped after the
initial local stores and before any compare/return path. BIR/prepared-BIR
showed the local subobject address was already published (`%t4` materialized
from the frame slot at offset 7); the first bad point was the next local array
byte load and sign-extension chain:
`bir.load_local i8 %lv.arr.7`, `bir.sext i8 ... to i32`, and later
`bir.load_local i8 ... addr %t31` through the pointer local.

RV64 prepared local memory emission now handles `i8` loads from frame slots and
from prepared pointer-value accesses. RV64 prepared scalar cast emission now
materializes `i8` sign/zero extension to `i32` into either a prepared register
home or a 4-byte stack home. This keeps the repair on the local array/subobject
access rule rather than adding filename, local-name, or offset-specific
handling.

Added focused backend coverage in
`tests/backend/case/riscv64_prepared_local_array_i8_element_access.c` for a
local `char[2][4]`, pointer-to-subarray, pointer-to-element, direct element
load, pointer-value element load, and adjacent `int[4]` check. The dump,
codegen-route, and rv64 runtime tests assert the prepared BIR shape, emitted
`lb` paths, and end-to-end qemu success.

Runtime movement under
`build/rv64_c_testsuite_probe_latest/triage_312_step4_00130/`: `00130`,
`00032`, and `00072` all pass emit, clang, and qemu.

## Suggested Next

Run the Step 6 acceptance sweep for idea 312 candidates, then decide whether
remaining failures are aggregate/function-pointer local flow for Step 5 or
out-of-scope follow-up work.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- `00032` is now an end-to-end proof for local array pointer reassignment and
  pointer-step materialization.
- `00072` remains the clean end-to-end proof for base local address
  publication.
- `00130` is now an end-to-end proof for constant local subobject address
  publication, byte element loads, pointer-value byte loads, and narrow
  extension before branch compares.
- `00143` remains too broad for this packet; keep it as indexed local
  array/select-update-chain follow-up evidence.

## Proof

Proof run:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_prepared_local_array_(base|subobject|pointer_step|i8_element)'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Results:

- Build passed.
- Focused local array subset passed `10/10`.
- Backend subset produced `test_after.log`: `passed=229 failed=1 total=230`.
  The only failing test is the pre-existing
  `backend_riscv_prepared_edge_publication`.
- Regression guard passed with `delta passed=3 failed=0` and no new failures.
- Runtime probe results are in
  `build/rv64_c_testsuite_probe_latest/triage_312_step4_00130/probe_results.tsv`.
