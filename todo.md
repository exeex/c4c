Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time completed the first real
return-lowering packet. Prepared `Return` terminators now lower to one
canonical selected return `MachineInstruction`, while branch/non-return
terminators remain diagnostic-only and emit no nodes.

The focused tests now cover direct dispatch, `build_prepared_module`, module
skeleton traversal, and function traversal. They continue to require
compatibility `machine_nodes` to stay empty; selected return nodes live only in
canonical MIR block instructions.

## Suggested Next

Supervisor can review and commit the coherent Step 5 return-lowering slice, or
delegate the next Step 5 lowering family once the commit boundary is accepted.

## Watchouts

- The current repo has no `MachineOpcode::Return`; this slice uses the existing
  selected return representation: `InstructionRecord` family `Return` with a
  `ReturnInstructionRecord` payload. The generic MIR opcode remains the target
  record's current opcode value.
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp` was delegated
  but does not exist as a real source file in this checkout, so no branch-control
  source split was made.
- Compatibility `machine_nodes` remain empty; selected return nodes live only in
  canonical MIR block instructions.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering'`

Result: passed; focused subset ran 6/6 tests. Proof output is preserved in
`test_after.log`.
