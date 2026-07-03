Status: Active
Source Idea Path: ideas/open/565_prepared_move_bundle_widening_stack_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Widening Move-Bundle Boundary

# Current Packet

## Just Finished

Completed plan Step 1, `Inspect Widening Move-Bundle Boundary`, for
`ideas/open/565_prepared_move_bundle_widening_stack_authority.md`.

Fresh representative proof:

- `build/agent_state/565_step1_widening_boundary.allowlist`
- `build/agent_state/565_step1_widening_boundary.log`

Current first bad facts:

- `src/20010224-1.c` still fails with
  `unsupported_prepared_move_bundle_classification`,
  `diagnostic_owner=prepared_move_bundle_classifier`, and
  `fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`.
  The move is `consumer_stack_to_stack` at
  `function=ba_compute_psd`, block `for.cond.1`,
  `instruction_index=2`, `phase=before_instruction`,
  `authority=none`, `parallel_copy=no`, from value `46` to value `47`.
  Source and destination homes are both `stack_slot`; source type is `i16`
  with `source_size_bytes=2`; destination type is `i32` with
  `destination_size_bytes=4`.
- `src/pr87623.c` still fails with the same diagnostic owner and fragment
  status. The move is `consumer_stack_to_stack` at
  `function=a_or_b_different`, block `logic.end.12`,
  `instruction_index=0`, `phase=before_instruction`,
  `authority=none`, `parallel_copy=no`, from value `7` to value `9`.
  Source and destination homes are both `stack_slot`; source type is `i8`
  with `source_size_bytes=1`; destination type is `i32` with
  `destination_size_bytes=4`.

Owning boundary remains prepared producer/classifier state. The rejection is
reported by
`src/backend/mir/riscv/codegen/object_emission.cpp::rv64_prepared_move_bundle_classification_failure_diagnostic(...)`,
but that helper is only formatting the prepared fact state. The bundle is
published as `consumer_stack_to_stack` with `PreparedMoveAuthorityKind::None`
through the prepared value-location/regalloc path:
`src/backend/prealloc/regalloc/consumer_moves.cpp::append_consumer_moves(...)`,
`src/backend/prealloc/regalloc.cpp::normalize_prepared_move_publication(...)`,
and `src/backend/prealloc/regalloc.cpp::append_prepared_move_bundle(...)`.
The generic prepared consumer classifier in
`src/backend/prealloc/prepared_object_traversal.cpp::classify_prepared_object_move_bundle_consumer(...)`
then sees a non-parallel `authority=None` bundle as otherwise structurally
available, leaving RV64 to fail-close with the classifier-owned diagnostic.

## Suggested Next

Advance to plan Step 2 with focused prepared/backend coverage for the semantic
widening shape before repairing it. The next packet should add a test that
constructs or observes a `BeforeInstruction` stack-slot source to stack-slot
destination conversion-adjacent move where the destination integer width is
larger than the source width, and asserts that the old state is rejected with
`authority=none` until prepared authority is made explicit.

Prefer coverage that drives the repair toward an explicit prepared split into
conversion plus stack-destination facts, or an equivalently explicit prepared
widening authority, rather than reusing the existing stack-to-stack byte-copy
path. The current RV64 stack-slot-to-stack-slot helper only accepts
`source_size_bytes >= destination_size_bytes`; these representatives require
`i16 -> i32` and `i8 -> i32`, so a plain stack copy would be the wrong semantic
model.

## Watchouts

- Keep this as prepared move-bundle classifier work unless inspection proves
  prepared authority is already coherent before RV64 consumption.
- Do not route these rows to RV64 while `prepared_move_bundle_classifier`
  reports `authority=none`.
- Do not special-case representative filenames, source widths, event names, or
  diagnostic strings.
- Do not touch expectations, unsupported markers, allowlists, or pass/fail
  accounting.
- Existing RV64 object-emission coverage in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` intentionally
  rejects a conversion-adjacent stack-to-stack shape today; Step 2 coverage
  should either add a focused prepared-authority contract or update that
  fixture only when the prepared producer repair is ready.

## Proof

Inspection-only packet. Ran:

```sh
printf '%s\n' src/20010224-1.c src/pr87623.c > build/agent_state/565_step1_widening_boundary.allowlist && ALLOWLIST=build/agent_state/565_step1_widening_boundary.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/565_step1_widening_boundary.log 2>&1
```

Result: expected representative failures, `total=2 passed=0 failed=2`, with
both rows still in
`unsupported_prepared_move_bundle_classification`. No `test_after.log` was
written because this packet was inspection-only and did not run backend tests.
