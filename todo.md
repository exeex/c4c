Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time added common MIR block
successor metadata and populated it for the currently lowered AArch64
terminators. `MachineBlock` now carries typed successor records with target
label, edge kind, and optional `MachineOrigin` provenance. AArch64 return
blocks record no successors, while prepared unconditional branch blocks record
one `Unconditional` successor using the prepared destination label and
`BirTerminator` provenance.

The slice keeps conditional/fused compare branch control fail-closed: unsupported
conditional terminators still emit no instruction and no successor metadata. It
does not restore legacy module records or use source text, compatibility
machine nodes, or testcase-name matching as successor authority.

## Suggested Next

Supervisor can review and commit this Step 5 common successor metadata slice,
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
- `MachineBlockSuccessorKind` has explicit conditional true/false edge kinds
  for a later packet, but this slice intentionally populates only unconditional
  branch successors.
- Conditional and fused-compare branch records already exist, but module
  lowering intentionally does not select them or attach successor metadata in
  this packet.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 8/8 tests. Proof output is preserved in
`test_after.log`.
