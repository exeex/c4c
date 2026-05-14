Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Function Traversal And Operand Resolution

# Current Packet

## Just Finished

Step 4: Implement Function Traversal And Operand Resolution completed the first
focused traversal slice. `build_prepared_module` now validates the AArch64
handoff, walks `prepared.control_flow.functions` in prepared order, creates
canonical `Module::mir.functions` and `MachineBlock` entries with preserved
prepared function/block identities, and leaves block instruction vectors empty
until semantic instruction lowering lands.

This packet also added a typed operand-resolution scaffold with function-local
prepared context and fail-closed diagnostics. It can resolve typed regalloc and
storage-plan register placements, simple immediate, label, symbol,
frame-slot/immediate storage, and rematerializable immediate value-home
operands, but it does not use broad legacy records, register spellings, cached
display strings, or flat machine-node vectors as semantic authority.
Compatibility `Module::functions` and
`Module::compatibility.functions` are minimal derived mirrors with empty
`machine_nodes`.

## Suggested Next

Delegate the next Step 4 packet to add block-local semantic instruction
dispatch scaffolding on top of the new traversal context. Keep unsupported
operation families fail-closed through typed diagnostics, and only emit
canonical `MachineInstruction` nodes when a real semantic lowering rule is
implemented.

## Watchouts

- `Module::mir` is now the canonical function/block traversal product; keep
  compatibility mirrors derived after canonical traversal and keep
  `machine_nodes` empty until real lowering selects printable instructions.
- The current operand scaffold deliberately fails closed for register
  value-homes that only have diagnostic spelling. Add typed placement/register
  authority before consuming those paths.
- Public assembly without selected machine nodes still fails closed; keep that
  behavior until semantic lowering populates canonical machine instructions.
- Do not restore deleted legacy module-record side tables, broad records,
  cached display-string authority, or source-spelling recovery.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_module_skeleton|backend_aarch64_mir_carrier|backend_aarch64_function_traversal|backend_aarch64_operand_resolution'`

Result: passed; focused subset ran 4/4 tests. Regression guard compared the
matching `test_before.log` and `test_after.log` and passed. Proof output is preserved in
`test_after.log`.
