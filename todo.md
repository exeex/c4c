Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time fixed the return-value
regression from the first return-lowering commit. Retained BIR return values
now feed `ReturnInstructionRecord::value`: void returns stay bare, immediate
integer returns materialize through typed immediate operands, and named returns
resolve through prepared value homes/storage/register authority.

The slice also selects the existing scalar add/sub record for return-selected
scalar results when prepared return ABI register authority exists, so generated
AArch64 assembly again emits the value-producing `mov`/`add` sequence before
`ret`. Compatibility `machine_nodes` now expose selected non-return nodes needed
by existing scalar coverage while return nodes remain canonical MIR-only.

## Suggested Next

Supervisor can review and commit this regression-fix slice, then continue with
the next Step 5 lowering family.

## Watchouts

- The current repo has no `MachineOpcode::Return`; this slice uses the existing
  selected return representation: `InstructionRecord` family `Return` with a
  `ReturnInstructionRecord` payload. The generic MIR opcode remains the target
  record's current opcode value.
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp` was delegated
  but does not exist as a real source file in this checkout, so no branch-control
  source split was made.
- Compatibility `machine_nodes` intentionally exclude return nodes but now carry
  selected non-return records such as return-selected scalar ALU nodes for the
  existing compatibility test surface.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_prepared_scalar_alu_records|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke'`

Result: passed; focused subset ran 9/9 tests. The generated smoke assembly
contains `mov w0, #0` for return-zero and `mov w0, #2` plus `add w0, w0, #3`
for return-add. Proof output is preserved in `test_after.log`.
