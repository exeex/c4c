Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move i128 construction and lowering bodies

# Current Packet

## Just Finished

Step 3 moved the behavior-preserving prepared i128 record-builder cluster from
`instruction.cpp` into `i128_ops.cpp`. The i128 owner now contains the
`make_prepared_i128_*` transport, copy, pair, shift, compare, and runtime-helper
boundary record builders, their i128-local record-error adapters, lane/operand
builder helpers, opcode classifiers, and opcode-to-i128-kind mapping helpers.

The moved compare builder now uses the existing scalar result-register builder
API instead of dragging generic anonymous scalar-home/storage helpers out of
`instruction.cpp`. Prepared carrier/runtime-helper authority remains unchanged.
`instruction.cpp` no longer contains the i128 prepared-record builder bodies.

## Suggested Next

Step 3 should move the remaining i128 lowering/printing bodies into
`i128_ops.cpp`/`i128_ops.hpp`: the dispatch lowering helpers and printer helpers
that consume the now-owned i128 record types. Keep this as a redistribution
packet only; do not add missing i128 semantics.

## Watchouts

- Do not move or redesign the prepared authority in `src/backend/prealloc/*`;
  i128 relocation should consume `PreparedI128Carrier*` and
  `PreparedI128RuntimeHelper*` facts exactly as the current broad owners do.
- `i128_ops.hpp` already exposes the prepared-record builders; reintroduce
  printer/lowering declarations only in the packet that actually moves those
  bodies.
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
