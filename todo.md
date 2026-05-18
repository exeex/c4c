Status: Active
Source Idea Path: ideas/open/282_aarch64_loop_branch_control_runtime_hang.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Conditional Branch Ownership

# Current Packet

## Just Finished

Step 1 localized the `00005.c` first-`if` straight-line branch failure.
The source and prepared control-flow facts are correct: BIR has
`bir.cond_br i1 %t2, block_1, block_2`, and prepared control flow records
`entry terminator=cond_branch true=block_1 false=block_2` with a fused
`ne i32 %t1, 0` branch condition. The generated AArch64 assembly preserves the
prior local pointer repair (`add x9, sp, #0`, `str x9, [sp, #8]`, distinct
frame-slot offsets), but after loading the first condition it falls straight
through to the true-return path:

```asm
ldr x20, [sp, #8]
ldr w20, [sp]
mov w0, #1
ret
```

There is no emitted `cmp`, `cbnz`/`cbz`, or conditional/unconditional branch
pair to route the false edge to `block_2`. The likely owner is terminal
AArch64 branch printing for prepared fused-compare conditional branches:
`src/backend/mir/aarch64/codegen/comparison.cpp::lower_prepared_conditional_branch_terminator`
and `src/backend/mir/aarch64/codegen/instruction.cpp::branch_selection_status`
already select/model a `MachineOpcode::CompareBranch` with
`BranchConditionForm::FusedCompare`, but
`src/backend/mir/aarch64/codegen/machine_printer.cpp::print_branch` only prints
materialized-bool conditional branches and rejects non-`MaterializedBool`
condition records. That printer gap explains why the branch-control record does
not reach real assembly and execution falls into `mov w0, #1; ret`.

## Suggested Next

Execute Step 2 from `plan.md`: repair fused-compare conditional branch terminal
printing so selected AArch64 branch records emit truthful control flow instead
of being omitted. The focused repair proof command should be:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005)_c$'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not reopen idea 281 address-exposed pointer storage unless trace evidence
  proves the exact owner returned.
- Do not weaken c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or timeout policy.
- Do not add filename-specific handling for `00005.c` or `00006.c`.
- Use timeout-bounded runtime proof or assembly-only localization while the
  `00006.c` hang remains active.
- Preserve prior green routes for `00001.c`, `00002.c`, `00003.c`, and
  `00004.c`.
- Do not weaken fused-compare branch records into unsupported forms. The
  selected record is already represented; the missing support boundary is
  terminal AArch64 printing for that supported form.
- `00006.c` likely shares branch/control components, but this localization only
  proves the `00005.c` first-`if` owner.

## Proof

Step 1 proof command:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00005_c$'; } 2>&1 | tee test_after.log
```

Result: FAIL, exit 8. `c_testsuite_aarch64_backend_src_00005_c` reaches the
AArch64 backend runtime route and fails as `[RUNTIME_NONZERO] exit=1`, matching
the localized straight-line first-`if` owner. `test_after.log` contains the
full command output.
