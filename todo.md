Status: Active
Source Idea Path: ideas/open/551_rv64_move_bundle_materialization_from_classified_bucket.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Stack-To-Stack Move Coverage

# Current Packet

## Just Finished

Completed Step 4, `Add Stack-To-Stack Move Coverage`.

`src/backend/mir/riscv/codegen/object_emission.cpp` now consumes coherent
stack-slot to stack-slot prepared move facts for the two selected
`consumer_stack_to_stack/stack_slot_to_stack_slot` representatives:

- stack-source and stack-destination homes must both remain coherent prepared
  GPR frame slots
- the source side validates the full prepared source stack slot size while the
  emitted load/store transfer width follows the prepared destination scalar
  type
- local-slot stack homes without a BIR scalar value type may use prepared
  value-home/frame-slot size authority instead of failing as missing type
  authority
- widening stack copies remain rejected; the supported rows are same-size or
  truncating copies from a prepared source slot into a prepared destination
  slot

No prepared-authority rows, expectation files, unsupported markers, allowlists
outside `build/agent_state`, or runtime comparison code were changed.

## Suggested Next

Proceed to Step 5 select-publication register move coverage for the remaining
coherent select-publication row, unless the supervisor chooses to reconcile the
already advanced representative subset first.

## Watchouts

- The Step 4 representatives now advance past
  `fragment_status=generic_move_bundle_materialization_failed`, but both
  currently stop later at `unsupported_terminator_fragment`.
- The stack-to-stack helper intentionally still rejects rows missing prepared
  destination type, rows with source capacity smaller than the destination
  transfer width, and rows whose source or destination stack home cannot be
  validated through prepared stack layout authority.
- Prepared-authority-gap rows such as same-shape rows with missing source or
  destination type should stay with the prepared authority follow-up, not be
  pulled into this implementation route.

## Proof

- Delegated Step 4 proof command was run exactly; full output is preserved in
  `test_after.log`.
- `cmake --build --preset default` completed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345 backend tests.
- The two-case RV64 gcc torture allowlist was `src/990127-1.c` and
  `src/pr20527-1.c`.
- The final assertion reports `generic_move_bundle_failure_count=0`.
- Per-case statuses were `advanced_or_pass`; both case logs show the current
  later residual as `unsupported_terminator_fragment`.
