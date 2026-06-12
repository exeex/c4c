Status: Active
Source Idea Path: ideas/open/209_route4_publication_source_semantic_reader.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Name the Reader and Baseline the Existing Route 4 Boundary

# Current Packet

## Just Finished

Step 1 selected exactly one Route 4 publication-source semantic reader:
`find_prepared_indirect_callee_source_producer(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Reader classification:
- This is a publication-source semantic consumer because it answers "which
  same-block producer created the selected indirect callee value before this
  call boundary" from `edge_publication_source_producers`, then passes only the
  selected source value/instruction boundary to later materialization.
- It is not publication mechanics or output policy: register choice, CSEL
  materialization, scratch selection, call record emission, wrapper spelling,
  and final printed instructions remain in
  `materialize_indirect_call_callee_to_prepared_register(...)`,
  `emit_indirect_callee_value_to_register_with_csel(...)`, and the existing
  AArch64 printer path.

Baseline surface:
- Prepared fallback source:
  `PreparedFunctionLookups::edge_publication_source_producers`, specifically
  `producers_by_value_name`, queried by value name, current prepared block
  label, and `before_instruction_index`.
- Prepared result identity:
  `prepared_indirect_callee_source_producer_result(...)` accepts only named
  producer results and resolves the result name through prepared name tables
  before returning the prepared producer.
- Route 4 evidence source:
  build a current-block `Route4PublicationAvailabilityIndex` over the retained
  BIR block and validate the selected value with
  `route4_validate_current_block_publication_reference(...)`, matching the
  existing `route4_call_boundary_source_identity(...)` pattern.
- Required agreement before accepting Route 4 for this reader:
  value spelling, optional value-name id, value type, prepared block/current
  BIR block relationship, source producer instruction pointer, source producer
  instruction index, and `before_instruction_index`.
- Prepared fallback must remain for missing, stale owner/wrong block,
  wrong-reference, duplicate reference, route/prepared mismatch, invalid
  producer, unsupported producer kind, and all materialization/output policy
  failures.

Current coverage baseline:
- Positive Route 4 current-block publication identity:
  `call_boundary_stack_source_uses_route4_identity_with_prepared_fallback()`
  proves the existing call-boundary reader can use Route 4 without prepared
  source-producer lookup.
- Absent and route-only rejection/fallback precedent:
  Route 6 scalar source-producer coverage exercises absent and duplicate route
  facts with prepared fallback or route-only rejection, but not this Route 4
  indirect-callee reader.
- Wrong-reference/stale owner:
  existing call-boundary Route 4 coverage mutates the BIR block label and
  proves prepared fallback, then fail-closed behavior without prepared lookup.
- Duplicate mismatch:
  `route4_validate_current_block_publication_reference(...)` has duplicate
  reference and wrong-key statuses, and helper coverage observes Route 4
  current-block identity agreement, but this selected reader does not yet have
  a duplicate/mismatch consumer test.
- Prepared fallback for selected indirect callee:
  `block_dispatch_falls_back_to_prepared_stored_indirect_callee_policy()`
  proves adjacent indirect-callee materialization retains prepared fallback for
  Route 3 stored-value source policy, but not Route 4 source-producer identity.
- Output/wrapper stability:
  `block_dispatch_uses_route3_stored_indirect_callee_identity_for_selected_source()`
  and
  `block_dispatch_falls_back_to_prepared_stored_indirect_callee_policy()`
  currently assert stable `cmp`, `csel x9`, and `blr x9` ordering for selected
  indirect callees.

## Suggested Next

Step 2 should add route/prepared agreement for the selected reader only:
`find_prepared_indirect_callee_source_producer(...)` may accept Route 4 current
block publication identity only when it agrees with the prepared producer
record, and otherwise must return the existing prepared producer result.

Recommended narrow proof command after implementation:
`cmake --build build --target backend_aarch64_instruction_dispatch_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

## Watchouts

Do not migrate or rename `prepared_call_boundary_source_value(...)`; it is the
already-migrated adjacent Route 4 reader and should remain a reference pattern.
Do not move indirect-callee materialization, scratch selection, ABI register
policy, CSEL emission, call records, wrappers, printer/debug rows, expected
strings, or prepared API surfaces into Route 4. The missing Step 2/3 proof cases
for this exact reader are positive route/prepared agreement, absent Route 4,
stale/wrong-reference Route 4, duplicate Route 4 reference, route/prepared
mismatch, and no-change `cmp`/`csel x9`/`blr x9` output.

## Proof

Analysis-only selection packet; no build required and no build run. Inspected
with `c4c-clang-tool-ccdb` plus targeted source/test reads. Proof surface for
later implementation is the recommended narrow command above, with canonical
output in `test_after.log`.
