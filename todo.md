Status: Active
Source Idea Path: ideas/open/76_aarch64_publication_ordering_evidence_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Edge-Copy Publication Ordering

# Current Packet

## Just Finished

Completed `plan.md` Step 1 evidence tracing for the representative
block-entry out-of-SSA edge-copy publication path.

Representative path traced:
`dispatch.cpp::lower_block` first emits normal block-entry moves via
`lower_value_moves(...)`, but `should_emit_block_entry_edge_copy_move(...)`
can suppress a redundant prepared parallel-copy move when
`prepare::prepare_edge_copy_source_facts(...)` proves that an edge publication
already covers the same source/destination move. At branch terminator time,
`lower_predecessor_select_parallel_copy_sources(...)` then emits the
predecessor-side source publication before the unconditional branch by
calling `lower_predecessor_join_source_publication(...)`.

Prepared fact owners/helpers in order:
`prepare::make_edge_publication_source_producers(...)` indexes BIR
`LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, and `SelectMaterialization`
producers by prepared value name. `prepare::make_prepared_edge_publication_lookups(...)`
walks prepared `join_transfers` and `edge_transfers`, binds
`PreparedEdgePublication::predecessor_label`, `successor_label`,
`destination_value`, `source_value`, destination value/home via
`find_indexed_prepared_value_id(...)` and
`find_indexed_prepared_value_home(...)`, source value/home the same way,
source producer metadata via `apply_source_producer_fact(...)`, optional
load-local memory metadata via `apply_source_memory_access_fact(...)`, the
published parallel-copy bundle via
`find_published_parallel_copy_bundle_for_edge_transfer(...)`, the matching
out-of-SSA move via `find_edge_publication_move(...)`, and prepared
parallel-copy step metadata via `find_edge_publication_parallel_copy_step(...)`.
`prepare::prepare_edge_copy_source_facts(...)` copies those fields into
`PreparedEdgeCopySourceFacts` and validates availability with
`validate_prepared_edge_copy_publication_source_facts(...)`.
`prepare::prepare_block_entry_parallel_copy_edge_source_facts(...)` further
binds the fact to the concrete `PreparedMoveResolution` for the block-entry
parallel-copy move and rejects edge or move mismatches.

AArch64 consumer/record path:
`lower_predecessor_select_parallel_copy_sources(...)` accepts only a
`PreparedMovePhase::BlockEntry` move bundle with
`PreparedMoveAuthorityKind::OutOfSsaParallelCopy`, matching predecessor and
successor labels, a value-to-value register destination move, and prepared
edge-copy facts with materializable producer, source home, destination home,
and matching move/source authority. It calls
`lower_predecessor_join_source_publication(...)`, which chooses the concrete
destination register from the prepared destination home, chooses only a
target-local scratch register, materializes either
`prepare::prepare_same_width_i32_stack_source_publication(...)` or
`emit_edge_value_publication_to_register(...)`, records source and destination
register aliases in `BlockScalarLoweringState`, and emits the AArch64
publication record through `make_select_chain_materialization_instruction(...)`.
That final helper constructs an `InstructionRecord` with
`InstructionFamily::Assembler`, `RecordSurfaceKind::MachineInstructionNode`,
selected status, function/block/instruction coordinates, and an
`AssemblerInstructionRecord::inline_asm_template` assembled by appending the
target-local `lines` vector in order.

Classification:
- Publication order: prepared/shared for edge identity and parallel-copy
  scheduling authority. `PreparedEdgePublication` carries
  `parallel_copy_bundle`, `parallel_copy_step_index`,
  `parallel_copy_step_kind`, `parallel_copy_step_uses_cycle_temp_source`,
  `parallel_copy_execution_site`, and `parallel_copy_execution_block_label`.
  AArch64 does not reorder those prepared steps for this path; it only decides
  whether to replace one proven block-entry move with a predecessor-side source
  materialization.
- Source binding: prepared/shared. Source value, source value id/name, source
  home/kind, source producer kind/block/instruction/pointer, and load-local
  prepared memory access facts are created in
  `make_prepared_edge_publication_lookups(...)` and copied through
  `prepare_edge_copy_source_facts(...)` /
  `prepare_block_entry_parallel_copy_edge_source_facts(...)`.
- Destination binding: prepared/shared for value/home/register authority.
  Destination value id/name, destination home/kind/storage, and matching
  `PreparedMoveResolution` are prepared facts. AArch64 parses the prepared
  destination register spelling to choose the concrete AArch64 view.
- Record emission order: target-local by design. The final instruction text
  order is the order in which `emit_same_width_i32_stack_source_publication(...)`
  or `emit_edge_value_publication_to_register(...)` appends strings to `lines`,
  and `make_select_chain_materialization_instruction(...)` serializes that
  vector into the record. This is target-local AArch64 spelling/sequence
  construction, not a shared publication-order query.
- Missing shared authority: none proven for this representative edge-copy
  path. The missing piece, if later traces find one, would be a shared
  publication-order query for record text sequencing; this edge-copy path
  already has shared authority for edge identity, source binding,
  destination binding, move binding, and parallel-copy step metadata.

## Suggested Next

Trace one representative call-boundary publication from prepared call or
publication facts to AArch64 records and compare its ordering authority against
the edge-copy model above.

## Watchouts

- Do not change implementation files during the evidence-only trace.
- Do not add local ordering helpers before authority is proven.
- Keep publication expectation changes out of this packet.
- For Step 2, distinguish prepared call-boundary effect ordering from
  target-local record spelling the same way this packet distinguishes prepared
  edge/move/step authority from AArch64 `lines` emission.

## Proof

No build required by delegated proof; evidence-only `todo.md` update. No
`test_after.log` was produced because the packet explicitly required no build
or test proof.
