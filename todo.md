Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time extended return-selected
scalar add/sub lowering from one immediate binary operation to a same-block
linear scalar ALU chain. The AArch64 module lowering now keeps block-local
emitted scalar value registers, derives the return ABI accumulator from the
prepared before-return move authority, retargets that register through linear
same-block add/sub results, and makes the final return consume the emitted
register instead of rematerializing over the selected chain.

The slice proves `tests/backend/case/return_add_sub_chain.c` through the public
AArch64 asm smoke without named-case matching: generated assembly contains
`mov w0, #2`, `add w0, w0, #3`, `sub w0, w0, #1`, and `ret`, and the external
run expects rc 4.

## Suggested Next

Supervisor can review and commit this Step 5 scalar-chain slice, then continue
with the next bounded AArch64 lowering family.

## Watchouts

- The current repo has no `MachineOpcode::Return`; this slice uses the existing
  selected return representation: `InstructionRecord` family `Return` with a
  `ReturnInstructionRecord` payload. The generic MIR opcode remains the target
  record's current opcode value.
- The return-chain fallback is intentionally narrow: it only retargets the
  prepared return ABI register through a same-block linear add/sub chain ending
  at the block return. It still fails closed for non-scalar, non-linear, or
  non-return-selected shapes.
- Compatibility `machine_nodes` still intentionally exclude return nodes but
  carry selected non-return scalar records for the existing compatibility test
  surface.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_prepared_scalar_alu_records|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 10/10 tests. Proof output is preserved in
`test_after.log`.
