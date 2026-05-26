Status: Active
Source Idea Path: ideas/open/09_calls_preservation_republication_retirement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Preservation And Republication Authority

# Current Packet

## Just Finished

Step 1 - Audit Preservation And Republication Authority completed.

Audited the AArch64 call preservation/republication paths and prepared
call-plan records:

- Prepared semantic authority already present:
  - `make_call_preservation_candidates` and `build_call_preserved_values`
    in `src/backend/prealloc/call_plans.cpp` decide which values cross a
    call, which route they use (`CalleeSavedRegister` vs `StackSlot`), and
    which prepared register, frame slot, spill slot, size, align, and saved
    callee index identify the preservation home.
  - `select_prepared_call_argument_source` decides later call-argument reuse
    from a prior preserved value through
    `PreparedCallArgumentSourceSelectionKind::PriorPreservation`, including
    the preserved call block/instruction and copied preservation source facts.
  - `plan_prepared_call_boundary_effects` emits prepared effect records for
    `PreservationHomePopulation` before the call and
    `PreservationRepublication` after the call from
    `PreparedCallPreservedValue`.
  - prepared lookup helpers in `src/backend/prealloc/prepared_lookups.cpp`
    select unique prior preservation, first stack-preserved values for a
    call, later stack-preserved reload source, and block-entry republication
    reachability.

- AArch64 target location/emission authority:
  - `lower_before_call_moves` consumes prepared effects and emits outgoing
    stack-base setup, callee-saved population, explicit argument moves, and
    immediate ABI bindings.
  - `make_callee_saved_preservation_home_population` consumes prepared
    callee-saved preservation effects, but still chooses source operands from
    current value homes when populating the preservation register.
  - `lower_after_call_moves` emits explicit after-call result moves, then
    consumes prepared `PreservationRepublication` effects for callee-saved
    values.
  - `make_callee_saved_preservation_home_republication_instruction` consumes
    prepared republication effects, but still chooses the destination register
    from the current prepared value home when available.
  - `make_prior_preserved_call_argument_source` consumes prepared
    `PriorPreservation` source selection for later call arguments; its
    remaining authority is target operand construction from the prepared
    route/register/stack fields.
  - `make_selected_call_argument_source` and the selected frame-slot/local
    address helpers are target operand constructors for prepared source
    selections.
  - `order_before_call_moves_for_source_preservation`,
    `call_boundary_move_reloads_materialized_address`,
    `retarget_call_boundary_source_to_emitted_scalar`,
    `materialize_call_boundary_source_to_destination`, and
    `record_call_boundary_destination` are dispatch-time ordering/cache
    helpers, not prepared semantic authority, but they can mask missing
    prepared source facts by using emitted scalar state.

- Target-local semantic authority that should move into prepared records or be
  represented by prepared effects before retirement:
  - `publish_stack_preserved_call_values` selects first stack-preserved values
    for a call and writes them from emitted/current scalar registers into the
    stack preservation home before address materialization and before-call
    moves. This is post-planning preservation-home publication authority.
  - `find_prior_stack_preserved_value_before_instruction` plus
    `make_prior_stack_preserved_value_source` in `lower_value_moves` decide
    later non-call reloads from stack-preserved homes during AArch64 value
    move emission. The lookup is prepared, but the reload source decision is
    still target-local and not a prepared effect consumed like callee-saved
    republication.
  - `materialize_missing_frame_slot_call_arguments` handles later call
    arguments from stack/frame-slot sources when normal before-call moves did
    not materialize them. It consumes prepared source selection when present,
    but remains a target-local fallback path for stack-preserved later
    call-argument reuse.
  - `lower_value_moves` falls back through emitted immediates, prior stack
    preservation, value-home register, and value-home stack-slot sources. That
    broad source order is the reload/home authority to retire for preserved
    values.

- Coverage observed:
  - Prepared effect planning covers callee-saved and stack preserved intent in
    `tests/backend/mir/backend_call_boundary_effect_plan_test.cpp`.
  - Stack-preserved call payload/effect extent coverage exists in
    `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`.
  - Later call-argument reuse, cross-block prior preservation, callee-saved
    republication ordering, stack-preserved non-call scalar reuse, and
    stack-preserved later call-argument reuse are covered in
    `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.
  - Prepared prior-preservation source-selection contracts and reject-missing
    source-selection checks exist in
    `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`;
    lookup coverage for cross-block unique prior preservation exists in
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

- Coverage gaps / next proof targets:
  - Clobbered registers: prepared `clobbered_registers` are recorded and call
    records expose clobber policy, but Step 2 should prove preservation source
    and destination facts against a caller-saved clobber that is later reused.
  - Stack-preserved values: coverage exists, but source/destination authority
    is split between prepared lookups and AArch64 publication/reload fallback;
    Step 2 should add a focused prepared-record check before changing
    emission.
  - Cross-block later uses: lookup and dispatch tests exist for cross-block
    prior preservation, but stack-preserved block-entry/later non-call reloads
    need a prepared effect/home record proof rather than relying on
    `lower_value_moves`.
  - Later call-argument reuse: same-block, cross-block, first/second argument,
    and missing-selection rejection coverage exists; Step 2 should preserve
    these while moving the preserved source/destination facts into prepared
    records.

## Suggested Next

Step 2 should be a bounded prepared-record packet: extend prepared
preservation records/effects so each preserved value names the semantic
preservation source and preservation destination explicitly, starting with
stack-preserved values and caller-saved clobber reuse. Do not modify AArch64
emission yet except as needed for compile fallout; add/adjust focused prepared
record tests that prove the source, destination, route, and reason before
retiring `publish_stack_preserved_call_values` or the `lower_value_moves`
prior-stack fallback.

## Watchouts

- Keep source idea `09` unchanged unless durable source intent changes.
- Do not revive broad post-call overwrite or reload fallback behavior.
- Do not downgrade tests or mark supported call-preservation coverage
  unsupported.
- Treat `publish_stack_preserved_call_values`,
  `find_prior_stack_preserved_value_before_instruction`, and the
  `lower_value_moves` prior-stack branch as the main target-local authority to
  retire.
- Callee-saved register population/republication is closer to the intended
  model, but destination register selection still consults current value homes;
  avoid baking that late choice deeper into AArch64.
- `materialize_missing_frame_slot_call_arguments` is a later call-argument
  fallback and must not become the replacement semantic authority for prepared
  preservation.

## Proof

Audit-only packet; no build or tests run. No `test_after.log` was updated for
this packet because the delegated proof explicitly required no build unless
code changed.
