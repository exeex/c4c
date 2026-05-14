Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time added the first AArch64
branch/control module lowering slice. Prepared unconditional BIR branch
terminators now lower through the existing
`codegen::make_prepared_unconditional_branch_record` authority into selected
canonical MIR `Branch` machine instructions with `BirTerminator` origin, source
block identity on the instruction record, and prepared destination label
identity preserved in the branch payload.

The slice adds a small `branch_control_lowering.cpp` seam, wires it into module
dispatch, and keeps conditional/fused compare branch control fail-closed for a
later packet. It does not restore legacy module records or use source-text or
testcase-name matching.

## Suggested Next

Supervisor can review and commit this Step 5 unconditional branch-control slice,
then continue with the next bounded AArch64 lowering family, likely conditional
materialized-bool branch lowering or the next prepared scalar/control family.

## Watchouts

- `codegen::make_branch_instruction` derives its initial record location from
  the target payload; module lowering overwrites the selected instruction's
  `function_name`, `block_label`, and `block_index` with the source block
  identity while leaving the branch payload target untouched.
- Unconditional branch lowering requires retained BIR terminator authority so
  the prepared target label and BIR `BranchTerminator::target_label_id` can be
  checked by the existing record API.
- Conditional and fused-compare branch records already exist, but module
  lowering intentionally does not select them in this packet.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_prepared_branch_records|backend_aarch64_branch_compare_records|backend_aarch64_branch_compare_contract|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 13/13 tests. Proof output is preserved in
`test_after.log`.
