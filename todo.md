Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Legacy Module-Record Compile Tests

# Current Packet

## Just Finished

Step 3: Migrate Legacy Module-Record Compile Tests replaced the stale AArch64
legacy module-record tests that referenced deleted `OperandRecord`,
`FrameRecord`, broad module-side record piles, and `module.globals` APIs. The
old compile targets `backend_aarch64_prepared_module_identity`,
`backend_aarch64_prepared_operand_identity`,
`backend_aarch64_prepared_frame_control`, and
`backend_aarch64_prepared_data_identity` were removed from
`tests/backend/mir/CMakeLists.txt` and their legacy sources were deleted.

Replacement coverage is explicit rather than a silent downgrade:
`backend_aarch64_module_skeleton_contract` proves the fresh AArch64 module
skeleton validates the prepared handoff and returns an empty canonical module
product, while `backend_aarch64_mir_carrier` proves the common
hierarchical MIR carrier preserves function, block, instruction, and BIR origin
identity. Existing `backend_aarch64_prepared_handoff_gate`,
`backend_aarch64_machine_printer`, and
`backend_cli_aarch64_asm_no_machine_nodes_fails` continue to cover handoff,
selected machine-node printing, and public assembly fail-closed behavior.

## Suggested Next

Delegate Step 4 to implement the first focused AArch64 function traversal and
operand-resolution packet on top of the common MIR carrier. Keep it semantic:
walk prepared functions/blocks into `Module::mir` and add only the operand
contracts needed by that traversal slice.

## Watchouts

- Future traversal and operand-resolution contracts are intentionally TODOs for
  Step 4; do not bring back the deleted legacy module-record side tables to
  satisfy them.
- `Module::mir` is the canonical product for new AArch64 traversal. Existing
  lower-level codegen record tests still own codegen-helper behavior and were
  not weakened by this migration.
- Public assembly without selected machine nodes must keep failing closed until
  semantic lowering populates printable machine nodes.
- Do not use cached display strings, source spellings, broad public records,
  raw prepared/source views, or register strings as semantic lowering
  authority.

## Proof

Ran:
`cmake --build build -j`

Then ran:
`ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_prepared_handoff_gate|backend_aarch64_machine_printer|backend_cli_aarch64_asm_no_machine_nodes_fails|backend_aarch64_.*module|backend_aarch64_.*carrier'`

Result: passed; subset ran 5/5 tests. Proof output is preserved in
`test_after.log`.
