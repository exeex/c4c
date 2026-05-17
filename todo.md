Status: Active
Source Idea Path: ideas/open/262_aarch64_i128_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Delete the reconciled markdown shard

# Current Packet

## Just Finished

Step 5 deleted the reconciled
`src/backend/mir/aarch64/codegen/i128_ops.md` markdown shard after confirming
the behavior-preserving shard content now has compiled ownership in
`i128_ops.cpp`/`i128_ops.hpp`.

Represented compiled ownership now includes i128 transport/copy records,
low/high lane register and memory facts, add/sub/and/or/xor pair-operation
records, immediate shift records, scalar compare records, div/rem runtime
helper boundary records, selected-node construction and status diagnostics,
dispatch lowering entry points, and printer spelling for the i128 target
instruction records.

The deleted shard also contained legacy reference guidance for behavior that
was not a behavior-preserving move in this redistribution plan: unary neg/not,
multiplication, variable shifts, float/i128 conversion helper calls, direct
fixed-register accumulator helpers, and generic stack or indirect pair helper
surfaces. Those remain explicit gaps or separate initiatives rather than
Step 5 implementation work.

## Suggested Next

Step 6 should run the supervisor-selected validation and route review for the
completed redistribution, including broader proof if the supervisor decides the
instruction/dispatch/printer touch points need more than the focused backend
subset.

## Watchouts

- Step 5 intentionally did not change implementation `.cpp`/`.hpp` files; this
  packet only removed the reconciled markdown shard and recorded the ownership
  decision.
- Do not resurrect `i128_ops.md` as active implementation guidance. New i128
  semantic expansion should route through a fresh source idea or supervisor
  packet.
- Preserve `comparison.cpp` and `memory.cpp` as existing narrow owners unless
  the supervisor chooses to fold their i128-specific helpers into the shard in a
  later behavior-preserving move.
- Keep `i128_ops.hpp`'s current dependency shape: it exposes i128 lowering and
  printer entry points while avoiding the `module.hpp`/`instruction.hpp`
  include cycle through forward declarations and the underlying MIR instruction
  result type.

## Proof

Proof command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The backend build completed and all 139 selected `^backend_`
tests passed. Proof log path: `test_after.log`.
