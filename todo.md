Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move i128 spelling and printing helpers

# Current Packet

## Just Finished

Step 4 moved the behavior-preserving i128 spelling and printing helper cluster
from `machine_printer.cpp` into `i128_ops.cpp`/`i128_ops.hpp`.
`validate_i128_helper_move`, `append_i128_helper_move_line`,
`pair_low_register_name`, `pair_high_register_name`, and the five
`print_i128_*` entry points are now owned by the i128 shard.

`machine_printer.cpp` now includes `i128_ops.hpp` and remains a neutral variant
router for i128 transport, pair-operation, shift, compare, and runtime-helper
records. Emitted assembly spelling and diagnostic text were preserved.

## Suggested Next

Step 5 should delete the stale markdown shard once the supervisor confirms the
source idea no longer needs the redistributed i128 helper text as active
implementation guidance.

## Watchouts

- Do not move or redesign the prepared authority in `src/backend/prealloc/*`;
  i128 relocation should consume `PreparedI128Carrier*` and
  `PreparedI128RuntimeHelper*` facts exactly as the current broad owners do.
- `i128_ops.hpp` now exposes the pair/copy lowering entry points. It avoids the
  `module.hpp`/`instruction.hpp` include cycle by using the underlying MIR
  instruction type in `I128InstructionLoweringResult` and forward-declaring the
  module context types.
- Avoid treating `i128_ops.md` as a license to add missing semantics during the
  redistribution steps. Behavior-changing gaps such as unary neg/not,
  multiplication, variable shifts, and float/i128 conversion emission need
  explicit supervisor routing if they become implementation work.
- Preserve `comparison.cpp` and `memory.cpp` as existing narrow owners unless
  the supervisor chooses to fold their i128-specific helpers into the new shard
  in a later behavior-preserving move.
- The i128 printer owner now consumes `memory_address` and i128 compare
  spelling helpers through existing public owner APIs; keep that dependency
  direction unchanged unless Step 5 explicitly routes more code.

## Proof

Proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The backend build completed and all 139 selected `^backend_`
tests passed. Proof log path: `test_after.log`.
