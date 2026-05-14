Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time added the AArch64 fusable
fused-compare conditional branch lowering family. Prepared `FusedCompare`
conditional terminators with `can_fuse_with_branch=true` now lower through the
prepared branch record API into selected canonical Branch-family machine nodes
with `MachineOpcode::CompareBranch`, retained `BirTerminator`
origin/source-block identity, prepared true/false target labels, preserved
compare predicate and operand records, and two MIR block successors marked
`ConditionalTrue` and `ConditionalFalse`.

The slice keeps non-fusable fused-compare conditional branches fail-closed with
a typed diagnostic and no emitted branch instruction or successor metadata. It
does not restore legacy module records, change public assembly bridge/printer
behavior, or use source text/testcase names as branch authority.

## Suggested Next

Supervisor can review and commit this Step 5 fusable fused-compare conditional
branch slice, then choose the next bounded AArch64 lowering family or route a
separate public printer/external branch smoke packet when assembly exposure is
in scope.

## Watchouts

- Materialized-bool branch lowering still requires a prepared branch condition,
  a retained BIR `CondBranch` terminator, matching prepared/BIR target labels,
  and typed register authority for the condition value. Immediate or stack-only
  condition authority remains diagnostic-only for that path.
- Fusable fused-compare lowering relies on prepared branch condition records and
  retained BIR terminator identity as semantic authority. Non-fusable fused
  compares remain diagnostic-only/fail-closed.
- Public printer/external conditional branch smoke remains out of scope for this
  packet; the internal MIR node is selected, but no assembly bridge behavior was
  added here.
- `codegen::make_branch_instruction` still derives initial record location from
  the target payload; module lowering overwrites the selected instruction's
  `function_name`, `block_label`, and `block_index` with the source block
  identity while leaving branch payload targets untouched.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_prepared_branch_records|backend_aarch64_branch_compare_records|backend_aarch64_compare_branch_candidate_records|backend_aarch64_branch_compare_contract|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 12/12 tests. Proof output is preserved in
`test_after.log`.
