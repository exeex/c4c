Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Function Traversal And Operand Resolution

# Current Packet

## Just Finished

Step 4: Implement Function Traversal And Operand Resolution tightened the
retained BIR mapping used by the block-local dispatch scaffold. Function
context now maps a prepared control-flow function to retained BIR by prepared
function-name spelling, and block context maps prepared block labels to
retained BIR `Block::label_id` spelling or retained `Block::label` spelling.
Vector index is no longer dispatch authority.

The dispatch test now includes reordered retained BIR functions and blocks.
Prepared control-flow order differs from retained BIR order, but dispatch still
visits the matching retained BIR block's unsupported instructions by identity
and keeps canonical `MachineBlock::instructions` plus compatibility
`machine_nodes` empty.

## Suggested Next

Delegate the next Step 4 packet to connect the first real prepared terminator
or operation family to semantic instruction lowering. Start with a narrow
family that has structured prepared authority, and keep unsupported families
diagnostic-only rather than falling back to legacy records or rendered text.

## Watchouts

- `PreparedControlFlowBlock` is sufficient authority for terminator visitation
  even when retained BIR block mapping is absent. Do not require fixture-side
  BIR population just to dispatch a prepared terminator.
- Retained BIR instruction traversal is only an optional operation dispatch
  source; unsupported families remain diagnostic-only and must not create
  placeholder `MachineInstruction` nodes.
- Retained BIR mapping currently follows prepared traversal indexes and
  requires non-invalid retained BIR ids before dispatch. Do not reintroduce
  label-spelling or raw source-name recovery as authority.
- Missing prepared block-to-BIR mapping may be diagnostic when a retained BIR
  function exists, but it must not suppress prepared terminator dispatch.
- Unsupported operations and terminators intentionally record diagnostics while
  emitting no `MachineInstruction` nodes. Do not treat those diagnostics as
  public assembly success.
- Keep compatibility mirrors derived after canonical traversal and keep
  `machine_nodes` empty until real lowering selects printable instructions.
- The operand scaffold still fails closed for register value-homes that only
  have diagnostic spelling; add typed placement/register authority before
  consuming those paths.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_function_traversal|backend_aarch64_operand_resolution|backend_aarch64_instruction_dispatch|backend_aarch64_instruction_lowering'`

Result: passed; focused subset ran 3/3 tests. Regression guard compared the
matching `test_before.log` and `test_after.log` and passed. Proof output is
preserved in `test_after.log`.
