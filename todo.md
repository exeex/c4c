Status: Active
Source Idea Path: ideas/open/10_calls_printing_and_effects_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Machine-Node Printing Out Of Call Lowering

# Current Packet

## Just Finished

Completed Step 3 - Move Machine-Node Printing Out Of Call Lowering. Moved the
AArch64 aggregate call-boundary machine-node spelling/classifier helpers from
the call byval aggregate surface into the machine-node record owner, and removed
the call-lowering header dependency from the AArch64 machine printer.

Changed files:

- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Specific moves:

- `aggregate_stack_copy_load_mnemonic`,
  `aggregate_stack_copy_store_mnemonic`, `aggregate_stack_copy_scratch`,
  `aggregate_stack_copy_chunks`, `aggregate_register_lane_load_mnemonic`,
  `aggregate_register_lane_load_register`, `aggregate_register_lane_scratch`,
  `aggregate_register_lane_memory`,
  `aggregate_register_lane_memory_is_printable`,
  `aggregate_register_lane_printable_chunk`,
  `aggregate_register_lane_destination`, and
  `is_aggregate_register_lane_publication` now live in `instruction.*`.
- `calls.hpp` no longer declares those machine-node spelling/classifier helpers.
- `calls_byval_aggregates.cpp` retains byval semantic helpers such as
  `is_aarch64_byval_register_lane_move` and source collection.
- `machine_printer.cpp` now gets those helpers through `instruction.hpp` and no
  longer includes `calls.hpp`.

## Suggested Next

Delegate Step 4 as a bounded surface-retirement packet: audit remaining
call-family display/effect surfaces and retire or rename only stale
printing/effect declarations or includes, without moving call constructors or
semantic lowering behavior.

## Watchouts

- `make_call_instruction`, `make_call_boundary_move_instruction`, and
  `make_call_boundary_abi_binding_instruction` were intentionally left in
  `calls.cpp`; this packet did not change semantic call machine-node
  construction.
- `calls_moves.cpp` still uses the moved aggregate helpers for inline-assembler
  machine-node payload construction, but the helpers are now declared by the
  machine-node owner rather than by `calls.hpp`.
- No test expectations were changed.
- There is an unrelated untracked file under `review/` in the working tree;
  leave it untouched for this packet.

## Proof

Ran the supervisor-selected proof and captured output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_instruction_dispatch|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_records)$'`

Result: build passed; 6/6 selected tests passed.

Additional AST-backed check: `c4c-clang-tool-ccdb function-signatures` on
`instruction.cpp` confirms the moved public aggregate machine-node helper
definitions now live there.
