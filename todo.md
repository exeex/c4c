Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move i128 construction and lowering bodies

# Current Packet

## Just Finished

Step 2 created the compiled AArch64 i128-ops owner boundary without moving
behavior. `src/backend/mir/aarch64/codegen/i128_ops.hpp` now exposes the
future owner surface for the i128 naming, selected-node construction,
prepared-record builders, printer entry points, and lowering entry points from
the Step 1 inventory. `src/backend/mir/aarch64/codegen/i128_ops.cpp` exists as
the compiled owner translation unit and is wired into `src/backend/CMakeLists.txt`.

No i128 definitions were moved out of `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp` in this packet.

## Suggested Next

Step 3 should move the i128 enum/string helpers and selected-node construction
helpers from `instruction.cpp` into `i128_ops.cpp`, then include `i128_ops.hpp`
from the consumers that need those declarations. Keep the move
behavior-preserving and avoid changing the instruction hierarchy.

## Watchouts

- Do not move or redesign the prepared authority in `src/backend/prealloc/*`;
  i128 relocation should consume `PreparedI128Carrier*` and
  `PreparedI128RuntimeHelper*` facts exactly as the current broad owners do.
- `i128_ops.hpp` currently declares the future boundary API while the
  definitions still live in their existing broad owners. Include it only when a
  relocation packet makes a consumer depend on the new owner header.
- Avoid treating `i128_ops.md` as a license to add missing semantics during the
  redistribution steps. Behavior-changing gaps such as unary neg/not,
  multiplication, variable shifts, and float/i128 conversion emission need
  explicit supervisor routing if they become implementation work.
- Preserve `comparison.cpp` and `memory.cpp` as existing narrow owners unless
  the supervisor chooses to fold their i128-specific helpers into the new shard
  in a later behavior-preserving move.

## Proof

Proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The backend build completed and all 139 selected `^backend_`
tests passed. Proof log path: `test_after.log`.
