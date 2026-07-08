Status: Active
Source Idea Path: ideas/open/587_prepared_value_freshness_authority_mvp.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: AArch64 And Unwired Consumer Inventory

# Current Packet

## Just Finished

Step 6 completed the consumer inventory for the prepared value freshness MVP.
No implementation files were edited.

Wired consumers:

- Shared prepared call-plan publication in
  `src/backend/prealloc/call_plans.cpp`: call arguments publish
  `CallArgumentSource` candidates for direct homes, explicit publications,
  producer rematerialization, and unique complete prior preservation. Evidence:
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` covers selected,
  no-candidate, invalid, ambiguous, and unknown-use query statuses;
  `tests/backend/bir/backend_prepared_printer_test.cpp` proves producer
  freshness outranks prior preservation and prior preservation remains selected
  when it is the only valid source.
- RV64 prepared call emission in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`: prior-preserved GPR
  call arguments must have selected `PriorPreservation` freshness, and register
  sourced arguments with published candidates must select direct-home,
  explicit-publication, or producer-rematerialization freshness before emission.
  Evidence: `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  accepts selected prior-preservation freshness and rejects missing,
  ambiguous, and producer-outranked prior-preservation freshness.
- Shared prepared object move-bundle consumer in
  `src/backend/prealloc/prepared_object_traversal.cpp`, used by RV64 object
  emission in `src/backend/mir/riscv/codegen/object_emission.cpp`: move-bundle
  sources query `MoveBundleSource` freshness and fail closed on missing,
  invalid, ambiguous, or wrong-kind authority before returning `Available`.
  Production RV64 object emission passes `lookups.value_homes` into the shared
  classifier. Evidence: `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`
  proves the production classifier does not manufacture source freshness and
  covers missing/invalid/ambiguous/unsupported diagnostics; `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  expects `MissingMoveBundleSourceFreshness` with the diagnostic
  `prepared move-bundle source is missing freshness authority`.
- Shared store-source publication planning in
  `src/backend/prealloc/publication_plans.cpp`: available
  `PreparedStoreSourcePublicationPlan` rows publish and select
  `ProducerPublicationOperand` freshness for validated same-block source
  producers. Evidence: `tests/backend/bir/backend_prepared_printer_test.cpp`
  shows `source_freshness_status=selected` and
  `source_freshness_authority=producer_rematerialization` for binary and
  select-materialization store-source publication routes.

Unwired or deferred nearby consumers:

- RV64 object-emission value-home helpers such as
  `prepared_value_home_for`, `gpr_register_number_for_value`,
  `fpr_register_number_for_value`, and rematerializable-immediate reads still
  query prepared homes directly. Deferred because they are broad object-value
  consumers outside the MVP use kinds unless reached through the wired
  move-bundle classifier or prepared call emitter. Existing protection is local
  contract verification and prepared object consumer diagnostics for missing,
  ambiguous, unsupported, and incomplete homes in
  `src/backend/prealloc/prepared_object_traversal.cpp`, covered by
  `tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp`.
- RV64 frame-slot address/value and byval aggregate call-argument paths in
  `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` still rely on their
  structured source-selection validators and stack/aggregate payload checks
  unless freshness candidates are published for a register source. Deferred
  because this MVP only wired the stale-home-prone prior-preservation/register
  representative path; nearby frame-slot/byval families require a follow-up
  freshness policy for stack-source and aggregate-lane authority.
- AArch64 call consumers in `src/backend/mir/aarch64/codegen/calls.cpp` are
  not wired to `find_prepared_value_freshness_authority`. They still use
  `find_prepared_call_argument_publication_source_routing`,
  `make_selected_call_argument_source`, prior-preservation payload checks, and
  byval/register-lane completeness checks. Deferred because wiring AArch64
  would be a second target-consumer migration beyond the representative RV64
  path. Existing fail-closed evidence: diagnostics use
  `MissingValueAuthority`, including
  `AArch64 prior-preserved call argument requires prepared PriorPreservation source selection`
  and `AArch64 indirect byval call-argument publication requires complete prepared selected source bytes`,
  covered by `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
- AArch64 prepared object traversal in
  `src/backend/mir/aarch64/codegen/traversal.cpp` and
  `src/backend/mir/aarch64/module/module.cpp` reports
  `PreparedObjectConsumerContractViolation` categories from the shared object
  consumer diagnostics but is not a direct freshness consumer. Already
  protected by the shared classifier when the shared prepared traversal reports
  object-consumer violations. Evidence:
  `tests/backend/mir/backend_aarch64_function_traversal_test.cpp` checks the
  diagnostic category path.
- x86 scalar i32 call-argument handoff in `src/backend/mir/x86/x86.hpp` and
  `src/backend/mir/x86/module/module.cpp` uses Route6 consumed source records
  through `find_consumed_scalar_i32_call_argument_source_authority`, not the
  new freshness authority. Deferred because it is a Route6 source-record
  threading contract, not one of the MVP prepared freshness use kinds. Evidence:
  `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`
  checks the consumed scalar call-argument source authority and rejects missing
  authority.
- x86 prepared move-bundle and decoded-home handoff paths remain unwired to the
  shared freshness query. They are protected by existing handoff short-circuit
  and `MissingValueAuthority` diagnostics rather than claiming freshness
  selection. Evidence:
  `tests/backend/bir/backend_x86_handoff_boundary_scalar_smoke_test.cpp`,
  `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`, and
  `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp`.
- Shared-prealloc dependency operand authorities, edge-publication move
  consumers, select-carrier alias authorities, branch stack-load authorities,
  typed stack-source publications, recovered narrow store-source publication
  helpers, and pending store-global publication runs remain intentionally
  unwired. Deferred because Step 5 selected store-source publication as the
  only producer/publication operand route for this MVP. Existing evidence:
  `tests/backend/bir/backend_prepared_printer_test.cpp` keeps dependency
  operand stack-load routes fail-closed as `status=missing_stack_freshness`
  and only claims `ProducerPublicationOperand` freshness for the store-source
  rows listed above.

## Suggested Next

Proceed to Step 7: run the supervisor-selected acceptance validation and
prepare the closure inventory. The closure note should reuse this inventory,
then add the final source-kind/use-kind summary and any follow-up idea names
for AArch64 freshness migration, RV64 stack/byval freshness, x86 Route6
freshness integration, and broader shared-prealloc operand authority.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist changes, or named-testcase shortcuts.
- AArch64 and x86 were inventoried only; do not imply they consume the MVP
  freshness authority.
- RV64 object emission consumes move-bundle freshness through the shared
  classifier, while RV64 call emission consumes call-argument freshness
  directly.
- Store-source publication freshness is the only producer/publication operand
  route claimed for the MVP.
- Equal-rank matching authorities intentionally fail closed as
  `ambiguous_candidate`.

## Proof

No validation was required or run for this todo-only inventory packet. No
`test_after.log` was created because no implementation files were edited.
