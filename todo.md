Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move i128 construction and lowering bodies

# Current Packet

## Just Finished

Step 3 moved the behavior-preserving i128 naming/status and selected-node
construction cluster from `instruction.cpp` into the new i128 owner.
`i128_ops.cpp` now owns the i128 enum/string helpers, prepared i128 error-name
helpers, i128 selection-status helpers, and the five `make_i128_*_instruction`
selected-node constructors. `instruction.hpp` no longer duplicates those
declarations; it includes `i128_ops.hpp` after the core instruction records so
existing consumers keep the same public include surface.

The prepared i128 record builders remain in `instruction.cpp` for the next
Step 3 packet. The dispatch lowering bodies and printer helpers were not moved
in this packet.

## Suggested Next

Step 3 should move the prepared i128 record-builder cluster from
`instruction.cpp` into `i128_ops.cpp`: `make_prepared_i128_*`, their local
record-error adapters, lane/operand builder helpers, opcode classifiers, and
opcode-to-i128-kind mapping helpers. Keep the move behavior-preserving and keep
the existing prepared carrier/runtime-helper authority intact.

## Watchouts

- Do not move or redesign the prepared authority in `src/backend/prealloc/*`;
  i128 relocation should consume `PreparedI128Carrier*` and
  `PreparedI128RuntimeHelper*` facts exactly as the current broad owners do.
- `i128_ops.hpp` was trimmed to avoid exposing not-yet-moved anonymous printer
  and lowering helpers; reintroduce printer/lowering declarations only in the
  packet that actually moves those bodies.
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
