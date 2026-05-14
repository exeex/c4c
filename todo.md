Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Split Or Classify Compatibility Projection

# Current Packet

## Just Finished

Step 6 docs/reference cleanup updated the owned AArch64 contracts to match the
post-extraction codegen layout:
`codegen/{instruction,operands,alu,comparison,returns,dispatch,traversal,emit,compatibility_projection}.*`.

Stale `records.cpp/.hpp`, deleted module draft path, and old record-owner
references were removed or replaced in the owned docs. Remaining
`machine_printer.*` references are current-route descriptions or explicitly
marked as the temporary terminal compatibility printer deferred to idea 224.

## Suggested Next

Next packet: supervisor or plan owner should decide whether Step 6 now exhausts
the current runbook or whether another target-private docs/codegen cleanup
remains inside the active idea before closure/deactivation.

## Watchouts

- Do not recreate deleted `module/function_traversal.cpp`,
  `module/operand_resolution.cpp`, `module/instruction_lowering.cpp`,
  `module/branch_control_lowering.cpp`, or `module/call_lowering.cpp`.
- `machine_printer.*` is still live code today as the temporary terminal
  compatibility printer; deleting it before idea 224 lands would break the
  public AArch64 asm path.
- Idea 224 owns replacing this target-local terminal printer with common MIR
  traversal plus AArch64 target rendering hooks.
- The compatibility projection is now target-private codegen glue derived from
  canonical lowered MIR; do not add fallback lowering inputs there.
- Implementation files, tests, `plan.md`, and `ideas/open/*` were not touched.

## Proof

`rg -n 'records\\.(cpp|hpp)|codegen/records|module/(function_traversal|operand_resolution|instruction_lowering|branch_control_lowering|call_lowering|public_assembly_bridge|compatibility_projection)\\.cpp|module\\.cpp\\.md|module\\.hpp\\.md|stage[23]_.*\\.md' src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md src/backend/mir/aarch64/ALLOCATION_CONTRACT.md src/backend/mir/aarch64/BACKEND_CASE_BRINGUP_MATRIX.md src/backend/mir/aarch64/BACKEND_ENTRY_CONTRACT.md src/backend/mir/aarch64/BINARY_UTILS_CONTRACT.md src/backend/mir/aarch64/BIR_PREPARED_GAP_LEDGER.md src/backend/mir/aarch64/CLASSIFICATION_INDEX.md src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md || true`

Passed with no stale-reference hits. Docs-only proof log: `test_after.log`.
