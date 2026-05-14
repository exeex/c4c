Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time added the AArch64
materialized-bool conditional branch lowering family. Prepared
`MaterializedBool` conditional terminators now lower through the prepared branch
record API into selected canonical Branch-family machine nodes with
`MachineOpcode::ConditionalBranch`, retained `BirTerminator` origin/source block
identity, prepared true/false target labels, a typed register condition operand
resolved through existing operand authority, and two MIR block successors marked
`ConditionalTrue` and `ConditionalFalse`.

The slice keeps fused-compare conditional branches fail-closed with a typed
diagnostic and no emitted branch instruction or successor metadata. It does not
restore legacy module records, change public assembly bridge/printer behavior,
or use source text/testcase names as branch authority.

## Suggested Next

Supervisor can review and commit this Step 5 materialized-bool conditional
branch slice, then choose the next bounded AArch64 lowering family or route a
separate printer/fused-compare packet if public conditional branch assembly is
next.

## Watchouts

- Materialized-bool branch lowering requires a prepared branch condition, a
  retained BIR `CondBranch` terminator, matching prepared/BIR target labels, and
  typed register authority for the condition value. Immediate or stack-only
  condition authority remains diagnostic-only for this path.
- Fused-compare branch records still exist below the module layer, but module
  lowering deliberately rejects them in this packet so compare-branch selection
  and printer/assembly behavior can be handled separately.
- `codegen::make_branch_instruction` still derives initial record location from
  the target payload; module lowering overwrites the selected instruction's
  `function_name`, `block_label`, and `block_index` with the source block
  identity while leaving branch payload targets untouched.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_prepared_branch_records|backend_aarch64_branch_compare_records|backend_aarch64_branch_compare_contract|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 11/11 tests. Proof output is preserved in
`test_after.log`.
