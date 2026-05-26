Status: Active
Source Idea Path: ideas/open/10_calls_printing_and_effects_owner_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move Generic Effect Spelling To Shared Owners

# Current Packet

## Just Finished

Completed Step 2 - Move Generic Effect Spelling To Shared Owners. Added
`src/backend/mir/aarch64/codegen/effects.hpp` and `effects.cpp` as the AArch64
machine-effect/display owner, moved the named effect spelling and construction
helpers there, and removed their declarations/definitions from the broad
instruction and call-lowering surfaces.

Changed files:

- `src/backend/mir/aarch64/codegen/effects.hpp`
- `src/backend/mir/aarch64/codegen/effects.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/f128.cpp`
- `src/backend/mir/aarch64/codegen/i128_ops.cpp`
- `src/backend/CMakeLists.txt`
- `tests/backend/mir/backend_aarch64_memory_operand_records_test.cpp`
- `tests/backend/mir/backend_aarch64_scalar_record_contract_test.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`

Specific moves:

- `machine_effect_resource_kind_name`,
  `machine_side_effect_kind_name`, `machine_effect_from_operand`,
  `machine_prepared_value_def`, `machine_effects_from_operands`,
  `effects_from_prepared_call_clobbers`, and
  `effects_from_prepared_call_preserved_values` now live in `effects.*`.
- `register_display_name`, `occupied_register_views`, and
  `prepared_clobber_expected_view` moved from the misleading call/instruction
  surfaces into `effects.*`.
- `calls.cpp`, `f128.cpp`, `i128_ops.cpp`, and `instruction.cpp` now include
  `effects.hpp` for effect construction/spelling. The local i128 duplicate
  generic operand-effect builder was replaced with the shared owner.
- Tests that directly assert effect spelling now include `effects.hpp`.

## Suggested Next

Delegate Step 3 as a bounded printer-owner packet: move machine-node printing
helpers out of call-lowering surfaces into the appropriate AArch64 printer or
machine-node owner while preserving existing printer output.

## Watchouts

- `machine_printer.cpp` was intentionally not changed in Step 2.
- Do not move `make_call_instruction`,
  `make_call_boundary_move_instruction`, or
  `make_call_boundary_abi_binding_instruction` in Step 3; they construct
  selected machine nodes and should stay with call emission until a separate
  machine-node-constructor owner is explicitly planned.
- `register_class_from_bank` remains in `calls_common.cpp`; `effects.cpp` uses
  a private equivalent for prepared call effect construction so the new effect
  owner does not depend back on the call header.
- `clang-format` was unavailable in the environment (`command not found`).
- There is an unrelated untracked file under `review/` in the working tree;
  leave it untouched for this packet.

## Proof

Ran the supervisor-selected proof and captured output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_x86_call_boundary_effect_ordering|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$'`

Result: build passed; 5/5 selected tests passed.

Additional AST-backed check: `c4c-clang-tool-ccdb function-signatures` on
`effects.cpp` confirms the moved public helper definitions now live there.
