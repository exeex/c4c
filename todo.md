Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move i128 spelling and printing helpers

# Current Packet

## Just Finished

Step 3 moved the behavior-preserving i128 dispatch lowering cluster from
`dispatch.cpp` into `i128_ops.cpp`/`i128_ops.hpp`. The i128 owner now contains
the pair/copy lowering entry points, pair/transport/runtime-helper diagnostic
message helpers, runtime-helper lookup, and the i128-local BIR machine
instruction wrapper used by those lowering routes.

`dispatch.cpp` now remains a neutral caller for the i128 pair-operation and copy
routes. Prepared carrier/runtime-helper authority and lowering semantics remain
unchanged, and the existing `memory.cpp` i128 transport path stays with the
memory owner.

## Suggested Next

Step 4 should move i128-specific spelling and printing helpers from
`machine_printer.cpp` into `i128_ops.cpp`/`i128_ops.hpp`, leaving
`machine_printer.cpp` as a neutral caller/delegator for i128 spelling.

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

## Proof

Proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The backend build completed and all 139 selected `^backend_`
tests passed. Proof log path: `test_after.log`.
