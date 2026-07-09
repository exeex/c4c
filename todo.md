Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Stack-Home Authority Carriers

# Current Packet

## Just Finished

Step 2 traced the aggregate/sret/byval stack-home authority carriers for the
clean in-scope family, using `src/20020215-1.c` and cross-checking
`src/pr30185.c` plus `src/pr38969.c`. Evidence extracts:
`build/agent_state/633_step2_producer_trace.txt` and
`build/agent_state/633_step2_consumer_trace.txt`.

Producer functions for the BIR copy instructions:
- `BirFunctionLowerer::materialize_aggregate_param_aliases` in
  `src/backend/bir/lir_to_bir/aggregate.cpp` emits
  `*.aggregate.param.copy.<offset>` `LoadLocalInst` from the byval pointer
  parameter and a following `StoreLocalInst` into the local aggregate leaf.
- `BirFunctionLowerer::try_lower_local_store` byval aggregate branch in
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` emits
  `*.byval.copy.<offset>` `LoadLocalInst` from the byval pointer value and a
  following `StoreLocalInst` into the local leaf.
- `BirFunctionLowerer::lower_block_terminator` in
  `src/backend/bir/lir_to_bir/module.cpp` emits `*.ret.sret.copy.<offset>`
  copy pairs through `append_local_aggregate_copy_to_pointer` when returning an
  aggregate through `%ret.sret`.

Prepared carrier publication:
- `build_function_memory_accesses` in
  `src/backend/prealloc/stack_layout/coordinator.cpp` constructs
  `PreparedMemoryAccess` entries for the copy instructions through
  `build_pointer_indirect_access` before falling back to direct frame-slot
  access construction.
- For `src/20020215-1.c`, the pointer-side prepared accesses are
  `base=pointer_value`, `pointer=%p.s` or `%ret.sret`, selected offsets
  `0,2,3,4,5,6,7,8,16,18,19,20,21,22,23`, sizes `1,2,8`,
  `base_plus_offset=yes`, and `range_verdict=proven_in_bounds`.
- The store-source table records same-block producer freshness for the copied
  values: `source_producer=load_local`,
  `source_freshness_authority=producer_rematerialization`, and
  `source_freshness_proof=same_block_before_use`.
- Cross-checks in `pr30185` and `pr38969` have the same carrier shape:
  pointer-side byval/sret accesses with stack-slot homes and exact selected
  offsets/sizes, paired with frame-slot local leaf stores.

Fields already carried:
- source value and stored/result value: `PreparedMemoryAccess::result_value_name`
  or `stored_value_name`, plus same-block store-source freshness facts.
- pointer/home identity: `PreparedAddress::pointer_value_name` names `%p.s`,
  `%p.x`, `%p.y`, or `%ret.sret`; `PreparedValueHome` and
  `PreparedStackObject` identify the stack-slot home.
- frame slot/home slot: `PreparedValueHome::slot_id`,
  `PreparedFrameSlot::slot_id`, `PreparedFrameSlot::offset_bytes`, and matching
  `PreparedStackObject::object_id`.
- selected byte facts: `PreparedAddress::byte_offset`, `size_bytes`,
  `align_bytes`, and `can_use_base_plus_offset`.
- ABI role: currently carried indirectly by `PreparedStackObject::source_kind`
  values `byval_param` and `sret_param`.
- object extent: available through `PreparedValueHome::size_bytes` and
  `PreparedStackObject::size_bytes`; access range is visible as
  `PreparedAddress::provenance.requested_range`.

First missing or ambiguous boundary:
- Producer publication is partly present but not yet a single explicit
  memory-use authority predicate for aggregate stack-home local memory.
  The prepared accesses still print `layout_authority=unknown`, and the
  existing RV64 byval/sret helpers rely on stack-home shape plus
  `PreparedStackObject::source_kind` rather than a named authority predicate
  analogous to direct-global or string local-memory authority.
- The smallest semantic next boundary is prepared-layer verification or
  publication of an explicit stack-home local-memory authority predicate over
  this existing carrier shape. It should accept only pointer-value bases whose
  home, frame slot, object extent, ABI role, byte offset, size, and range match
  the selected byval/sret aggregate lane.

## Suggested Next

Step 3 code packet: add focused prepared-layer coverage, and if needed a small
producer-side helper, for explicit aggregate stack-home local-memory authority.
The packet should prove the byval `%p.s` load and sret `%ret.sret` store lanes
from `src/20020215-1.c`-shaped prepared data preserve pointer/home identity,
stack object source kind, frame slot, selected offset, size, alignment, complete
object extent, proven in-bounds range, and same-block copied-value freshness.
If a helper is added, keep it semantic and reusable, for example
`prepared_stack_home_local_memory_has_authority(...)`, rather than matching
source filenames or copy-name spelling.

## Watchouts

Fail closed for missing home identity, missing or stale copied source value,
missing frame slot, mismatched `PreparedStackObject::source_kind`, incomplete
offset/size/alignment, requested range not proven in bounds, ambiguous aggregate
lane, unsupported ABI role, non-default address space, volatile access, and
misclassified pointer-value homes.

Keep F128/16-byte rows, outgoing stack args, pointer-result calls,
return-destination-only rows, string/global rows, global-object traffic,
large-offset pointer rows, and move-bundle fan-in rows out of the first
prepared-authority packet.

## Proof

No build proof required for this trace-only packet. Used existing Step 1
prepared dumps and focused source/AST-backed queries; no root-level `.log` file
was created.
