Status: Active
Source Idea Path: ideas/open/565_prepared_move_bundle_widening_stack_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Widening Authority Coverage

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

Plan Step 2, `Add Focused Widening Authority Coverage`, is ready for an
executor packet.

Packet objective: add focused prepared/backend coverage for conversion-adjacent
integer widening moves from one stack slot to another, using semantic width and
storage facts rather than representative filenames or diagnostic text. The
coverage should construct or observe a `BeforeInstruction` move whose source
home and destination home are both `stack_slot`, whose destination integer
width is larger than the source width, and whose current prepared state reaches
the old `authority=none` rejection until an explicit prepared authority shape
or prepared conversion-plus-stack-destination split is implemented.

Required coverage shape:

- Cover both representative width families, `i8 -> i32` and `i16 -> i32`, or
  one width-general fixture that demonstrably subsumes both.
- Assert the prepared contract directly: widening authority must become
  explicit before RV64 object-route consumption, either as a dedicated prepared
  widening authority or as explicit prepared conversion plus stack-destination
  move facts.
- Preserve rejection coverage for unsupported move-bundle shapes so unrelated
  memory-to-memory copies do not become authorized.

Executor boundaries:

- Owned files: `todo.md`, focused backend/prepared tests, and only the directly
  adjacent prepared move-bundle test helpers needed to express the coverage.
- Do not touch `plan.md`, the source idea, `ideas/closed/`, `review/`, RV64
  object-emission implementation, unsupported markers, expected pass/fail
  accounting, or representative allowlists.
- Do not implement the full prepared repair unless the focused coverage cannot
  be compiled or expressed without a minimal contract surface; if that happens,
  keep the repair limited to publishing explicit prepared authority for the
  tested widening shape and record that Step 3 may already be partially or
  fully consumed.

Proof command requested for the executor:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Done when `test_after.log` records a passing backend subset and `todo.md`
states whether the coverage is red-only pending Step 3 repair, or whether a
minimal prepared-authority contract was also added and Step 3 should be
re-scoped.

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
