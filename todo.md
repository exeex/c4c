# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map value-materialization authority gaps

## Just Finished

Completed Step 1 audit for
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`.

First fallback route to repair: the non-edge scalar materialization path in
`emit_value_publication_to_register` that calls
`find_same_block_named_producer(context, value.name, before_instruction_index)`
and then recursively reconstructs producer semantics for cast, binary,
load-local, load-global, and select results. This should be repaired before the
more specialized load-global/select/local-slot routes because it is the main
source-of-truth fallback feeding the rest of the function.

Prepared authority already consumed:

- Immediate values and same-block integer constants are rematerialized directly.
- Current block-entry publications are consumed through
  `current_block_entry_publication_register`.
- Value homes are consumed through `prepared_value_home_for_value`,
  `value_has_current_block_entry_publication`,
  `emit_prepared_value_home_publication_to_register`, and
  `emit_prepared_value_home_to_register`.
- Load-local has prepared consumers for va-list fields, prepared global-symbol
  loads, recovered narrow-store sources via
  `find_prepared_recovered_narrow_store_source_for_wide_local_load`,
  stack offsets via `prepared_local_load_offset`, and pointer-value loads via
  `emit_prepared_pointer_value_load_to_register`.

Local reconstruction/fallbacks still present:

- `find_same_block_named_producer` remains the non-edge scalar producer
  selector for named values when block-entry/home facts do not answer.
- Recursive cast and binary lowering reselects operand producer chains locally;
  scratch-order checks also call same-block producer recursion.
- Load-global locally recovers target global, symbol label, and GOT/direct
  policy through `find_load_global_target`, `load_global_symbol_label`, and the
  BIR global policy.
- Select materialization falls back to
  `emit_select_chain_value_to_register` plus `prepared_named_value_id` rather
  than a non-edge scalar select-chain authority.
- Add/sub local-slot address materialization delegates to
  `emit_local_slot_address_publication_to_register`, which remains tied to the
  local-slot address authority gap noted by the source idea.

## Suggested Next

Implement Step 2 for the first route: route the non-edge
`emit_value_publication_to_register` named-value producer path through prepared
source/home authority. Start by checking whether
`PreparedEdgePublicationSourceProducerLookups` plus
`PreparedEdgePublicationSourceProducer`, `PreparedScalarPublicationPlan`, and
`PreparedValueHome` can cover the value/instruction-index context; add a narrow
shared scalar-materialization query only if the existing prepared edge producer
lookup is edge-only and cannot safely answer non-edge consumers.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this value
materialization repair with the open memory, ALU, calls, comparison, or other
AArch64 follow-up scopes. Reject deeper same-block producer recursion,
value-name scans, hard-coded global-name/GOT policy recovery, local-slot
spelling recovery, and expectation downgrades as progress.

The first code packet should preserve scratch/read-write hazard ordering while
moving semantic source selection away from local same-block producer lookup.
Block-entry and value-home branches are already prepared consumers and should
not be replaced with move-bundle or value-name scans.

## Proof

Step 1 was audit-only; no build required and no `test_after.log` produced.
Supervisor-selected future proof subset for the first repair route:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
