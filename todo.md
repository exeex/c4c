Status: Active
Source Idea Path: ideas/open/11_aarch64_calls_file_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Merge Safe Emission-Only Calls Helpers

# Current Packet

## Just Finished

Step 2 - Merge Safe Emission-Only Calls Helpers completed for the target
operand adapter consolidation. Moved the helper definitions formerly in
`src/backend/mir/aarch64/codegen/calls_operand_adapters.cpp` into
`src/backend/mir/aarch64/codegen/calls_common.cpp`, removed the retired source
from `src/backend/CMakeLists.txt`, deleted the old translation unit, and kept
the existing `calls.hpp` declarations public. Updated the `calls.hpp` grouping
comment from the retired filename to a target-adapter label only.

AST and search checks used:

- `c4c-clang-tool-ccdb list-symbols` and `function-signatures` on both
  `calls_operand_adapters.cpp` and `calls_common.cpp` before the move.
- `c4c-clang-tool-ccdb function-callers` from `calls_moves.cpp` and
  `calls_dispatch_bridge.cpp` for the moved adapter helpers; callers still
  cross translation-unit boundaries, so declarations were preserved.
- `c4c-clang-tool-ccdb find-definition` after the move confirmed
  `make_register_operand_from_prepared_authority` and
  `scalar_integer_register_view_from_size` now resolve in `calls_common.cpp`.
- `rg calls_operand_adapters` after CMake regeneration found no source/build
  metadata references; only the stale header grouping comment remained and was
  renamed.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_operand_adapters.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/CMakeLists.txt`
- `todo.md`
- `test_after.log`

## Suggested Next

Supervisor should review/commit this completed Step 2 adapter consolidation
slice. The next coherent implementation packet, if continuing this idea, is a
separate byval-helper owner check before considering any
`calls_byval_aggregates.cpp` consolidation; keep that packet isolated from
dispatch bridge or broad move-lowering changes.

## Watchouts

- This packet was mechanical consolidation only; no semantic lowering,
  dispatch bridge behavior, ABI classification, or tests were changed.
- `calls.hpp` declarations intentionally remain public because `calls_moves.cpp`
  and `calls_dispatch_bridge.cpp` still call moved helpers across translation
  units.
- Leave unrelated transient `review/` artifacts untouched.
- Do not fold `calls_dispatch_bridge.*`, `dispatch.cpp`, or broad
  `calls_moves.cpp` behavior into this consolidation route.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract)$'`

Result: build succeeded; 3/3 focused tests passed.
