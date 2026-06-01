Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft Prepared Aggregate-Transport Contract

# Current Packet

## Just Finished

Step 2 of `plan.md` drafted the prepared aggregate-transport contract in
`todo.md` only. Step 1 already established that shared authority is missing for
call-argument aggregate transport; this packet names the contract shape before
any implementation migration.

Proposed shared surface:

- Add a call-argument aggregate transport fact under the prepared call planning
  / call preallocation surfaces, adjacent to `PreparedCallArgumentPlan` and
  `PreparedCallArgumentSourceSelection` in `src/backend/prealloc/calls.hpp`.
  The variadic aggregate `PreparedVariadicAggregateVaArgAccessPlan` remains a
  separate access-plan surface, but is the precedent for publishing source
  class, payload size/alignment, copy size/alignment, slot/field offsets,
  lane count/size, and lane destination homes before target lowering.
- Suggested names: `PreparedAggregateTransportKind`,
  `PreparedAggregateTransportChunk`, `PreparedAggregateTransportLane`,
  `PreparedAggregateTransportScratchRequirement`, and
  `PreparedAggregateTransportPlan`.

Proposed transport kind:

- `stack_copy`: aggregate payload is copied between stack-backed homes.
- `byval_register_lanes`: aggregate payload is published into ABI argument
  register lanes, usually from a stack-backed source.
- `preserved_register_lanes`: aggregate lanes are already represented by
  preserved register homes and need publication facts rather than copy
  decomposition.
- `mixed`: aggregate transport combines stack bytes and one or more register
  lanes, or otherwise needs both chunk and lane facts to avoid target-local
  re-derivation.

Proposed identity and source fields:

- Function, block, instruction, call, and argument identity sufficient to query
  a plan from a call-boundary lowering point without name-based matching.
- Argument ordinal plus stable source-selection link back to
  `PreparedCallArgumentSourceSelection`.
- Source value identity when available, source home/storage class, source
  field or slot identity, source base offset, payload size/alignment, and
  source ABI classification facts already selected by prepared call planning.
- Whether the source is stack-backed, register-backed, immediate/constant,
  preserved from an earlier prepared move, or unavailable without target-local
  materialization.

Proposed destination ABI fields:

- Destination ABI stack home facts: stack slot or frame area identity,
  destination base offset, ABI stack alignment, and total stack publication
  size when the transport writes memory.
- Destination ABI register facts: destination register name/class as ABI data,
  lane home, lane byte offset, occupied-register set, and whether the lane is
  final ABI-visible state or an intermediate publication target.
- Copy size, copy alignment, payload size, payload alignment, and any padding
  or tail-byte policy required to keep target lowering from recalculating
  boundaries.

Proposed chunk descriptors:

- Chunk index, payload-relative offset, chunk size, required alignment, source
  byte offset, destination byte offset, and whether the chunk is required,
  padding, or fallback-only.
- Source descriptor: stack offset, register lane, preserved home, or
  source-selection reference.
- Destination descriptor: stack offset, ABI lane destination, preserved home,
  or implementation-required temporary destination.
- Printable/preferred width as an abstract width requirement, plus an allowed
  fallback width sequence if the target cannot directly spell the preferred
  memory operation. This must not name AArch64 load/store mnemonics.

Proposed lane descriptors:

- Lane index, lane size, lane payload offset, source offset, destination offset,
  destination register/name/home, and occupied registers participating in the
  lane group.
- Source lane/home when the lane is already register-backed.
- Required stack-publication chunks for lanes whose source is memory-backed.
- Whether a lane consumes a whole ABI register, a partial lane, or a preserved
  aggregate lane home.

Proposed scratch requirement class:

- Express scratch needs as requirements, not concrete target registers:
  general-purpose scratch width, floating/SIMD scratch width, address scratch,
  temporary lane materialization, and whether the scratch may overlap source or
  destination occupied registers.
- Include lifetime/ordering constraints when a chunk or lane needs a scratch
  across load and store, but leave exact AArch64 register identity to target
  lowering/regalloc.

Owner, query, and consumer responsibilities:

- Publisher: prepared call planning/regalloc after ABI classification and
  source selection, because only that layer sees both argument source homes and
  destination ABI homes before target lowering.
- Query shape: lookup by function/block/instruction/call identity plus
  argument ordinal, returning an optional `PreparedAggregateTransportPlan` for
  aggregate call arguments whose transport is stack-backed, byval
  register-lane-backed, preserved-register-backed, or mixed.
- AArch64 call-boundary lowering consumes the plan to avoid re-deriving
  transport kind, chunk boundaries, lane bindings, source/destination offsets,
  lane destination homes, occupied registers, fallback chunk strategy, and
  scratch requirements.
- AArch64 printer may inspect prepared transport facts or target records derived
  from them for stable debug/test visibility, but final text spelling remains
  target-local.

Target-local AArch64 responsibilities:

- Exact load/store mnemonic selection and instruction opcode choice.
- Concrete scratch register identity and register allocation constraints after
  reading abstract scratch requirements.
- Memory address spelling, printability decisions, and fallback assembly text.
- Inline assembly text, machine record construction, final printer formatting,
  and any AArch64-specific target instruction schema.
- Legalization of abstract chunk/lane requirements into concrete AArch64
  instructions without changing the shared contract facts.

Implementation reject conditions:

- Do not migrate AArch64 aggregate lowering until the contract is printed,
  dumped, record-tested, or otherwise inspectable enough to prove consumers are
  reading prepared facts instead of local re-derivation.
- Do not add named-case shortcuts keyed to specific aggregate tests, functions,
  argument ordinals, or ABI examples.
- Do not weaken or rewrite expectations to claim progress without prepared
  transport authority.
- Do not fold in general memory address cleanup, i128/f128 carrier cleanup, or
  unrelated helper consolidation.
- Do not accept a helper-only rename that leaves AArch64 still computing chunk
  widths, lane homes, offsets, occupied registers, and scratch needs locally.

## Suggested Next

If the supervisor accepts this contract draft, begin Step 3 with the smallest
implementation slice: add the inspectable prepared aggregate-transport data
types/query shape under the prepared call planning surfaces, without migrating
AArch64 consumers yet unless the proof includes contract visibility.

## Watchouts

- Keep variadic aggregate `va_arg` plans separate. They are a design precedent,
  not the call-argument transport plan itself.
- Do not present i128/f128 carrier transport as the aggregate solution; it is a
  useful prepared-record precedent, not the general aggregate chunk/lane
  authority.
- Step 3 should prove the contract through an inspectable dump, record, or
  focused query test before moving target lowering. Otherwise the migration can
  collapse back into unproven helper reshuffling.
- AArch64 remains responsible for concrete mnemonics, scratch register identity,
  address spelling, inline asm text, record construction, and final formatting.
- Do not weaken aggregate testcase expectations, add named-case shortcuts, or
  combine this with general memory cleanup.

## Proof

Contract-only `todo.md` update. No build or test run required by the delegated
proof contract, and no proof log was produced.
