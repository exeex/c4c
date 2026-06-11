Status: Active
Source Idea Path: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and document the Route 1 consumer

# Current Packet

## Just Finished

Step 1 selected the bounded Route 1 consumer for the next implementation
packet: `value_publication_may_read_register_index(...)` in
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

Selected consumer:

- The recursive value-publication dependency check currently calls
  `prepared_same_block_publication_source_producer(context, value,
  before_instruction_index)` before walking cast, binary, and select producer
  operands to decide whether publishing `value` may read a register that the
  current move/publication would clobber.
- This is a semantic producer-identity read only. The register/home question
  remains in AArch64 code through `prepared_value_home_for_value(...)`,
  `value_has_current_block_entry_publication(...)`, and
  `prepared_value_home_reads_register_index(...)`.

Prepared helper reads to compare against:

- `prepared_same_block_publication_source_producer(...)` first maps the BIR
  named value with `prepared_named_value_id(context, value)`.
- It then reads
  `prepare::find_prepared_same_block_scalar_producer(...)` using
  `context.function.prepared->names`,
  `context.function.prepared_lookups->edge_publication_source_producers`,
  `context.control_flow_block->block_label`, `context.bir_block`, the prepared
  value name id, `value.type`, and `before_instruction_index`.
- Its no-aggregate fallback builds
  `prepare::make_prepared_edge_publication_source_producer_lookups(...)` and
  repeats the same prepared scalar-producer query. Keep that as fallback/oracle
  behavior for the implementation packet.

Intended Route 1 key:

- Build or receive a Route 1 producer index for `*context.bir_block`.
- Query `bir::Route1SameBlockProducerQuery{.index = &route1_index,
  .before_instruction_index = before_instruction_index}` with the consuming
  `bir::Value value` from `value_publication_may_read_register_index(...)`.
- Accept only same-block, before-consumer producer records that match the
  requested value name and type through Route 1 validation. Do not rescan broad
  BIR, match names outside the route query, or copy prepared homes/registers
  into the Route 1 view.

Existing focused coverage to preserve/extend:

- `backend_aarch64_instruction_dispatch` covers this publication dependency
  family with
  `predecessor_add_publication_preserves_rhs_register_before_target_clobber`,
  `edge_publication_dependency_uses_prepared_root_producer`, and nearby
  fail-closed/authority coverage in
  `branch_condition_publication_uses_prepared_source_producer_authority`.
- `backend_prepared_lookup_helper` already covers Route 1 vs prepared producer
  and integer-constant oracle behavior for same-block producer, constant,
  no-producer, non-constant, future-producer, missing, and mismatched-type
  cases.

## Suggested Next

Build the narrow Route 1 consumer view for the selected
`value_publication_may_read_register_index(...)` path, keeping the prepared
same-block producer helper available as fallback/oracle.

## Watchouts

Keep the implementation limited to semantic producer facts for the selected
publication dependency walk. Do not change storage homes, register selection,
move bundles, emitted records, materialization spelling, broad BIR rescans,
name matching, or route-wide prepared API contraction. If integer-constant
facts are added to the shared view in Step 2, keep them semantic only; this
selected consumer itself does not need constant folding to decide register-read
dependency.

## Proof

Not run; discovery-only packet.

Proposed future proof for implementation packets:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'`
