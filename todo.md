Status: Active
Source Idea Path: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Minimal Consumer API Shape

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: defined the minimal target-neutral consumer
API/query shape for x86 and future RISC-V edge-publication consumers.

Smallest shared consumer contract:

- Consumers should read the existing
  `PreparedFunctionLookups::edge_publications` table produced by
  `make_prepared_function_lookups`, not add a new wrapper authority.
- Edge identity is the existing `PreparedEdgePublicationKey`:
  `(predecessor_label, successor_label, destination_value_id)`, queried through
  `find_indexed_prepared_edge_publications`,
  `find_unique_indexed_prepared_edge_publication`, or
  `find_unique_indexed_block_entry_parallel_copy_edge_publication` when the
  consumer starts from a block-entry parallel-copy move.
- Iteration is already available through
  `PreparedEdgePublicationLookups::publications`; consumers that need all
  publications for a function can filter entries with
  `status == PreparedEdgePublicationLookupStatus::Available`.
- Edge/block identity comes from `predecessor_label`, `successor_label`,
  `phase`, `parallel_copy_execution_site`, and optional
  `parallel_copy_execution_block_label`.
- Source meaning comes from `source_value`, `source_value_kind`,
  optional `source_value_id`/`source_value_name`, `source_home`,
  `source_home_kind`, and source producer facts such as
  `source_producer_kind`, `source_producer_block_label`, and
  `source_producer_instruction_index`.
- Destination/live-in meaning comes from `destination_value`,
  `destination_value_id`, `destination_value_name`, `destination_home`,
  `destination_home_kind`, and `destination_storage_kind`. This is the
  semantic live-in publication; target code may decode/render storage later.
- Ordering and cycle information should use the facts already present on
  `PreparedEdgePublication`: `parallel_copy_step_index`,
  `parallel_copy_step_kind`, `parallel_copy_step_uses_cycle_temp_source`,
  `parallel_copy_bundle_has_cycle`, `parallel_copy_execution_site`,
  `parallel_copy_execution_block_label`, and the `parallel_copy_bundle`,
  `move_bundle`, and `move` links when a target needs to align with existing
  move-bundle emission.
- `x86::ConsumedPlans::shared_function_lookups()` already exposes the correct
  shared object after `consume_prepared_function_lookups`; x86 can consume
  `shared_function_lookups()->edge_publications` directly. The older
  `x86::prepared::Query::collect_block_entry_publications(successor_label)` is
  still useful for narrow register-only block-entry publication reads, but it
  lacks predecessor identity, source value semantics, and cycle/ordering facts,
  so it should not become the cross-target edge-publication contract.

Explicit exclusions from the target-neutral API:

- Do not publish physical register names, scratch-register choices, register
  classes, stack operand spellings, or assembly operands as edge-publication
  identity.
- Do not encode x86/RISC-V move instruction spelling, temporary selection,
  clobber-save/restore sequencing, or ABI-specific scratch policy in the shared
  query shape.
- Do not create a second target-facing wrapper unless an implementation proves
  the existing `PreparedFunctionLookups::edge_publications` and indexed helper
  functions are insufficient.

## Suggested Next

Proceed to a minimal x86 consumer proof that reads
`ConsumedPlans::shared_function_lookups()->edge_publications` for an existing
prepared predecessor/successor edge and keeps all emission choices target-local.

## Watchouts

- Existing shared APIs already satisfy the minimal semantic query shape; avoid
  wrapper churn unless the next implementation slice exposes a concrete missing
  fact.
- x86 is ready for a minimal consumer proof through `ConsumedPlans`. RISC-V
  still lacks a prepared-module lowerer entry point and prepared
  function/block traversal, so its first consumer change should wait for that
  integration surface.
- `collect_block_entry_publications(successor_label)` is narrower than the
  desired edge-publication contract because it is successor-only and
  register-publication oriented.
- Scratch selection, clobber discipline, physical register spelling, stack
  operands, move instruction spelling, and assembly syntax remain target-local
  for both x86 and RISC-V.

## Proof

No build required per packet because this was an audit/design-only `todo.md`
update. Read-only inspection covered shared prepared lookup declarations and
construction, x86 prepared/consumer surfaces, x86 module handoff context, and
RISC-V codegen entry points. No root-level logs were created.
