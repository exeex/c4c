Status: Active
Source Idea Path: ideas/open/312_rv64_local_stack_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Cover And Repair Array Element Local Access

# Current Packet

## Just Finished

Step 4 implementation packet repaired the next local array pointer-step
boundary exposed by `src/00032.c`.

Fresh repro under
`build/rv64_c_testsuite_probe_latest/triage_312_step4/` showed `00032`
emitted and linked but qemu exited 139 with the repo runtime command shape. The
first bad point was `.Lmain_block_6`: emission stopped after loading the pointer
local because `%t27 = bir.sub ptr %lv.arr.0, 4` was unsupported. The prepared
facts already described this as a frame-slot address materialization with
`offset=-4`.

RV64 prepared pointer address emission now handles pointer-minus-integer using
the same semantic materialization path as pointer add. Frame-slot pointer
materializations accept signed 12-bit offsets, including negative offsets, and
the fallback stack-slot pointer path emits `addi` with a negated immediate or
`sub` for register offsets. Pointer-pointer subtraction remains unsupported.

Added compact backend coverage in
`tests/backend/case/riscv64_prepared_local_array_pointer_step.c` for a local
array pointer local that is reassigned, post-incremented, pre-decremented, and
post-decremented. The dump/route/runtime tests assert the `ptr - int` prepared
BIR shape, `sp - 4` materialization, and end-to-end qemu success.

Runtime movement under
`build/rv64_c_testsuite_probe_latest/triage_312_step4/`: `00032` now passes
emit, clang, and qemu; `00072` remains passing; `00130` still emits and links
but exits qemu 132.

## Suggested Next

Repair or reclassify the remaining `00130` qemu 132 tail. It is no longer the
constant subobject pointer publication fixed in Step 3 and did not move with
the pointer-minus-integer Step 4 repair.

## Watchouts

- Do not special-case candidate filenames, variable names, or fixed stack
  offsets.
- `00032` is now an end-to-end proof for local array pointer reassignment and
  pointer-step materialization.
- `00072` remains the clean end-to-end proof for base local address
  publication.
- `00130` now publishes the constant subobject address and links; its qemu 132
  residual appears beyond pointer add/sub address materialization.
- `00143` remains too broad for this packet; keep it as indexed local
  array/select-update-chain follow-up evidence.

## Proof

Proof run:

- `cmake --build --preset default -j`
- `ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_prepared_local_array_pointer_step'`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Results:

- Build passed.
- Focused pointer-step subset passed `3/3`.
- Backend subset produced `test_after.log`: `passed=226 failed=1 total=227`.
  The only failing test is the pre-existing
  `backend_riscv_prepared_edge_publication`.
- Regression guard passed with `delta passed=3 failed=0` and no new failures.
- Runtime probe results are in
  `build/rv64_c_testsuite_probe_latest/triage_312_step4/probe_results.tsv`.
