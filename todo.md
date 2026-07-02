Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Or Reclassify The Single-Move Shape

# Current Packet

## Just Finished

Step 3 - Materialize Or Reclassify The Single-Move Shape completed for idea
514.

RV64 object emission now keeps the existing same-width
`consumer_register_to_stack` materializer unchanged and adds a precise
classification-owner diagnostic for the conversion-aware mismatch observed in
`pr27073.c`: a single `consumer_stack_to_stack` stack-destination move whose
prepared source home is a register and whose source/destination scalar sizes
differ.

The handled reject boundary reports:

- `unsupported_prepared_move_bundle_classification`
- `diagnostic_owner=prepared_move_bundle_classifier`
- source and destination home kinds
- source and destination scalar types and byte sizes
- `fragment_status=producer_classification_rejected_register_source_stack_destination_conversion`

For `pr27073.c`, the old generic
`unsupported_move_bundle_target_shape`/`generic_move_bundle_materialization_failed`
gate is replaced by the precise classifier-owned diagnostic for
`consumer_stack_to_stack`, source home `register`, destination home
`stack_slot`, source type `i16`, destination type `i32`.

Focused backend coverage now asserts the synthetic `i16 -> i32`
register-source stack-destination mismatch rejects with the classifier-owned
diagnostic and does not contain
`generic_move_bundle_materialization_failed`.

## Suggested Next

Execute Step 4 by adding any remaining focused backend coverage the supervisor
wants around adjacent malformed register-source stack-destination facts, or
move to Step 5 validation/handoff if the current classifier-owned reject
coverage is sufficient.

## Watchouts

Remaining unsupported shapes stay fail-closed: same-width reason/source-home
mismatches still use the generic move-bundle materialization diagnostic,
conversion moves without register-source and stack-destination homes are not
reclassified by this packet, and no `consumer_stack_to_stack` materialization
was widened.

Keep `20010518-1.c` as the idea 516 guard. It still rejects at
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
before RV64 materialization.

## Proof

Proof is recorded in `test_after.log`.

Commands run:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/pr27073.c -o /tmp/c4c_514_pr27073.o`
- `./build/c4cll --codegen obj --target riscv64-unknown-linux-gnu tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_514_20010518.o`
- `git diff --check -- src/backend/mir/riscv/codegen/object_emission.cpp tests/backend/mir/backend_riscv_object_emission_test.cpp todo.md`

The two object commands intentionally fail as probes: `pr27073.c` now reports
`unsupported_prepared_move_bundle_classification` with
`diagnostic_owner=prepared_move_bundle_classifier`, and `20010518-1.c` remains
at
`prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
