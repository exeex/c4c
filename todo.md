Status: Active
Source Idea Path: ideas/open/76_aarch64_publication_ordering_evidence_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Trace Typed Stack-Source Publication Ordering

# Current Packet

## Prior Context

Step 1 traced the representative block-entry out-of-SSA edge-copy path.
Prepared/shared authority owns edge identity, source binding, destination
binding, move binding, and parallel-copy step metadata through
`make_prepared_edge_publication_lookups(...)`,
`prepare_edge_copy_source_facts(...)`, and
`prepare_block_entry_parallel_copy_edge_source_facts(...)`. AArch64 consumes
those facts in `lower_predecessor_select_parallel_copy_sources(...)` /
`lower_predecessor_join_source_publication(...)`; final AArch64 `lines`
serialization into the record remains target-local by design.

Step 2 traced the representative call-boundary path. Prepared/shared authority
owns call identity, argument/result/preservation facts, clobber facts, and
call-boundary effect ordering through `PreparedCallPlan`,
`PreparedMoveBundle`, `PreparedCallBoundaryEffectPlan`, and
`plan_prepared_call_boundary_effects(...)`. AArch64 consumes those facts in
`lower_before_call_moves(...)`, `lower_call_instruction(...)`, and
`lower_after_call_moves(...)`; hazard-driven materialization and concrete
record sequencing remain target-local by design.

## Just Finished

Completed `plan.md` Step 3 evidence tracing for one representative typed
stack-source publication path: a same-width `i32` `LoadLocalInst` or
`LoadGlobalInst` whose loaded value is later consumed as an edge-publication
stack source and whose destination is an out-of-SSA register move.

Representative path traced:
`lower_memory_instruction(...)` obtains `edge_publications` from
`context.function.prepared_lookups->edge_publications` and routes loads to
`make_prepared_load_memory_instruction_record(...)`. The load-local and
load-global overloads call
`find_unique_load_result_stack_source_publication(...)`, which scans
`PreparedEdgePublicationLookups::publications` for the unique publication with
the current predecessor block label and matching `source_load_local` or
`source_load_global`. That publication is passed to
`prepare::prepare_same_width_i32_stack_source_publication(...)`, then into
`make_load_memory_instruction_record(...)`. If the loaded result home is a
prepared stack slot, `make_load_result_stack_publication_scratch(...)` uses the
typed publication's prepared destination register placement as the scratch
register. The final `MemoryInstructionRecord` is converted by
`make_memory_instruction(...)`, then wrapped by
`make_bir_machine_instruction(...)`.

Prepared owners/helpers consumed by AArch64 memory lowering:
`prepare::make_prepared_edge_publication_lookups(...)` supplies the publication
set indexed and consumed as `PreparedEdgePublicationLookups::publications`.
Each `PreparedEdgePublication` carries predecessor/successor identity,
destination value identity, optional source value identity, source producer
links such as `source_load_local` / `source_load_global`, `source_home`,
source memory facts, destination storage kind, destination home, and the
prepared `move`. `prepare::prepare_same_width_i32_stack_source_publication(...)`
is the shared typed stack-source adapter: it requires an available edge
publication, a stack-slot source home with concrete slot/offset/size/align
facts, same-width `I32` source and destination types, a register destination,
and an out-of-SSA move from the source value id to the destination value id
with prepared destination register placement in the GPR bank. It then records
`source_type`, `destination_type`, source/destination ids and names,
`source_stack_offset_bytes`, `source_stack_size_bytes`,
`source_stack_align_bytes`, `destination_register_bank`,
`destination_register_placement`, and
`PreparedTypedStackSourceExtensionPolicy::SameWidthNoExtension`.

AArch64 consumer/record path:
`make_prepared_memory_operand_record(...)` builds the load address and result
identity from prepared addressing and value-home data. For stack-homed load
results, `make_load_memory_instruction_record(...)` calls
`make_load_result_stack_publication_scratch(...)`; when the typed stack-source
publication is available, that helper converts
`typed_stack_source.destination_register_placement` with
`abi::convert_prepared_register(...)` and constructs the load result
`RegisterOperand` with `RegisterOperandRole::SpillAuthority`. The resulting
`MemoryInstructionRecord` stores the prepared load address, result value
identity, result type, chosen result register, and stack result offset. After
`apply_stack_layout_to_memory_record(...)`,
`retarget_load_result_to_return_abi(...)`, and
`apply_frame_pointer_base_policy(...)`, `make_memory_instruction(...)` emits a
single `InstructionFamily::Memory` / `MachineOpcode::Load` record. Its
operand list is the address operand only; its def is the result register when
present, otherwise the prepared value; its uses are derived from the address
operand by `memory_effects_from_operands(...)`.

Classification:
- Stack source: prepared/shared. The source is the unique prepared edge
  publication found by predecessor label plus `source_load_local` or
  `source_load_global`; concrete stack source facts come from
  `PreparedEdgePublication::source_home` and are normalized by
  `prepare_same_width_i32_stack_source_publication(...)` into slot, offset,
  size, and align fields. AArch64 does not rediscover the stack source, but it
  does choose to use the typed publication only for the same-width `i32`
  stack-source load-result path.
- Type: prepared/shared. The shared adapter checks
  `publication->source_value.type` and `publication->destination_value.type`
  are both `bir::TypeKind::I32`, records those types, and sets
  `SameWidthNoExtension`. AArch64 maps the result type to an AArch64 scalar
  register view and load spelling through `scalar_storage_register_view(...)`
  and normal memory lowering helpers; that is target-local spelling over the
  prepared type contract.
- Destination binding: prepared/shared. The required destination register is
  the out-of-SSA `PreparedMoveResolution::destination_register_placement` from
  the edge publication, validated by
  `prepare_same_width_i32_stack_source_publication(...)` and consumed by
  `make_load_result_stack_publication_scratch(...)`. AArch64 converts the
  prepared placement into an ABI register reference, but does not choose the
  destination value or register-placement authority.
- Publication order: prepared/shared for publication identity only through the
  existing edge-publication facts; no separate typed stack-source publication
  order is present in the memory path. `find_unique_load_result_stack_source_publication(...)`
  searches for the unique relevant prepared publication by current block label
  and source load, not by local instruction-order heuristics. Because this
  representative path folds the typed stack-source publication into the load
  record, there is no additional typed publication sequence to order after the
  prepared edge publication has been selected.
- Record emission order: target-local by design. `lower_memory_instruction(...)`
  emits the load record at the BIR memory instruction position after prepared
  addressing, stack-layout, return-ABI, and frame-pointer adjustments. Within
  the record, `make_memory_instruction(...)` orders operands as address then
  optional value, derives uses from operands, and emits the load def as the
  prepared scratch/result register. Those choices are AArch64 record shape and
  target spelling.
- Missing shared authority: no missing shared semantic authority is proven for
  this representative same-width `i32` typed stack-source path. The typed path
  differs from Step 1 and Step 2 because it has no explicit effect-order list:
  it consumes an existing prepared edge publication and shared typed adapter,
  then embeds the publication destination into the load record. The shared
  authority covers source, type, destination, and edge-publication identity;
  target-local code covers record construction and final emission position.
  If a later path needs multiple typed stack-source publications independent
  of edge-publication identity, that would be a new missing-query question, but
  this representative path does not prove that gap.

## Suggested Next

Run `plan.md` Step 4: compare the three recorded paths and decide whether
existing prepared edge/call/typed facts are sufficient or whether a new
prepared publication-order query is needed.

## Watchouts

- Do not change implementation files during the evidence-only trace.
- Do not add local ordering helpers before authority is proven.
- Keep publication expectation changes out of this packet.
- Step 4 should compare unlike ordering surfaces carefully: Step 1 has
  prepared edge-copy source facts, Step 2 has an explicit shared
  call-boundary effect planner, and Step 3 has prepared edge-publication
  identity plus a typed adapter but no separate typed publication-order list.

## Proof

No build required by delegated proof; evidence-only `todo.md` update. No
`test_after.log` was produced because the packet explicitly required no build
or test proof.
