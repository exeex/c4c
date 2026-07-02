Status: Active
Source Idea Path: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Materialize Or Reject Conversion-Adjacent Movement

# Current Packet

## Just Finished

Step 4 - Materialize Or Reject Conversion-Adjacent Movement added the
owner-specific RV64 rejection for prepared stack-slot source to stack-slot
destination moves whose BIR scalar types differ and whose move bundle carries
no explicit conversion materialization contract.

`src/backend/mir/riscv/codegen/object_emission.cpp` now separates that
conversion-adjacent shape from the generic move-bundle materialization failure:
the diagnostic starts with
`unsupported_prepared_move_bundle_classification`, reports
`diagnostic_owner=prepared_move_bundle_classifier`, includes the stack-slot
home kinds plus source/destination scalar types, and ends with
`fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`.
The existing register-source stack-destination conversion rejection remains
preserved, and the same-scalar stack-slot copy materializer still owns the
accepted raw stack-copy path.

`tests/backend/mir/backend_riscv_object_emission_test.cpp` now covers a
conversion-adjacent stack-to-stack fixture and asserts that both the text-object
builder and ELF writer preserve the classifier-owned diagnostic without
falling through to `generic_move_bundle_materialization_failed`.

Focused `pr69447.c` proof now reports the owner-specific
`i16 -> i64` stack-source/stack-destination conversion-adjacent diagnostic for
the prepared move bundle, exits nonzero as unsupported, and leaves no output
object behind. No raw stack-copy lowering was added.

## Suggested Next

Proceed to the next Step 4/5 packet by deciding whether any explicit
conversion materialization contract exists for prepared stack-slot movement, or
move to coverage/closure if unsupported conversion-adjacent stack-slot moves
are intentionally fail-closed for this idea.

## Watchouts

Remaining unsupported shapes are intentionally outside this packet: actual
stack-slot conversion materialization, bankless storage-plan endpoints, missing
scalar type authority, non-stack homes, multi-source/non-parallel stack
destinations, and parallel-copy or cycle-temp stack movement. The new
diagnostic is deliberately limited to a single prepared
`consumer_stack_to_stack` move with stack-slot homes and mismatched known scalar
types.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- Focused `pr69447.c` object-emission probe, expected nonzero, failing closed
  with `unsupported_prepared_move_bundle_classification`,
  `diagnostic_owner=prepared_move_bundle_classifier`,
  `source_type=i16`, `destination_type=i64`, and
  `fragment_status=producer_classification_rejected_stack_source_stack_destination_conversion_adjacent_move`;
  the output object is absent.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`
