Status: Active
Source Idea Path: ideas/open/44_shared_prepared_dynamic_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Dynamic Stack-Source Authority

# Current Packet

## Just Finished

Step 1 of `plan.md`: audited dynamic stack-source authority at the prepared
edge-publication boundary.

Observed dynamic stack-source shapes that reach prepared edge-publication
planning:

- `PreparedEdgePublication` can carry a `source_home` whose
  `source_home_kind` is `PreparedValueHomeKind::StackSlot` while
  `PreparedValueHome::offset_bytes` is absent. This reaches
  `prepare_same_width_i32_stack_source_publication()` and the RISC-V
  `consume_edge_publication_move_intent()` path, but current shared typed
  stack-source authority rejects it as `MissingConcreteStackSource`.
- The same `StackSlot` source can carry `pointer_base_value_name` and
  `pointer_byte_delta` fields while `offset_bytes` is absent. Current RISC-V
  coverage proves this remains `UnsupportedSourceHome` and does not get
  reclassified as pointer-base materialization.
- `PreparedValueHomeKind::PointerBasePlusOffset` also reaches prepared edge
  publication planning, but it represents an address value publication
  (`addi`/`mv`/large-delta `li; add`), not a stack-source memory load. It is a
  nearby unsafe inference route, not the dynamic stack-source family.

Current shared facts:

- Base anchor: concrete stack sources have implicit frame/SP authority via
  `slot_id` plus `offset_bytes`; dynamic `StackSlot` sources currently have no
  shared base-anchor enum or validated base register. Pointer-base fields name
  a value, but for `StackSlot` they are not load authority.
- Address expression: concrete stack sources have `offset_bytes`; dynamic
  `StackSlot` sources may have `pointer_base_value_name` and
  `pointer_byte_delta`, but no prepared fact says this expression is the source
  load address for the edge publication.
- Source memory access: `PreparedEdgePublicationSourceProducer` records
  producer kind and pointers for `LoadLocal`, `Cast`, `Binary`, and
  `SelectMaterialization`; it does not attach the `PreparedMemoryAccess` for a
  `LoadLocal` source. `publication_plans.cpp` has a nearby helper pattern that
  can recover a `LoadLocal` producer's `PreparedMemoryAccess` from addressing,
  but edge-publication facts do not expose it yet.
- Load width: concrete stack-source authority currently uses
  `source_home->size_bytes` and type checks. Dynamic sources do not have a
  shared source-load width from `PreparedMemoryAccess::address.size_bytes`
  attached to the edge publication.
- Validity: available edge-publication lookup plus OutOfSsa parallel-copy move
  authority is present, but dynamic address validity is not. Missing concrete
  offset stays fail-closed.
- Scratch needs: concrete register-destination stack loads use no explicit
  shared scratch for signed-12-bit offsets and target-local `t6` for large
  offsets. Dynamic address loads have no shared scratch contract yet, so RISC-V
  must not assume one.

Unsafe inference routes to avoid:

- Do not treat `StackSlot` plus `pointer_base_value_name`/`pointer_byte_delta`
  as a valid dynamic load address without a shared memory-access fact.
- Do not reinterpret `PointerBasePlusOffset` edge publications as memory
  loads; they are address-value materializations.
- Do not scan predecessor/successor blocks in RISC-V to rediscover a
  `LoadLocal`, base register, or stack address.
- Do not infer `sp + delta` from absent `offset_bytes`, fixture names, value
  ids, or pointer-base decorations.
- Do not add subword loads until signedness/extension authority exists.

Selected Step 2 target: `LoadLocal`-produced dynamic `StackSlot` source where
the source value of the edge publication has producer kind `LoadLocal` and the
shared addressing lookup can attach the exact `PreparedMemoryAccess` for that
load. The producer gap to close first is that `PreparedEdgePublication` does
not currently carry source memory-access facts: base kind/name, byte offset,
size/alignment, validity, and scratch/address-materialization requirements.

## Suggested Next

Step 2 should add shared prepared authority for the selected `LoadLocal`
dynamic stack-source family, or explicitly fail closed when the `LoadLocal`
producer lacks a matching `PreparedMemoryAccess`.

## Watchouts

RISC-V must continue to consume only shared prepared facts. Keep concrete
offset stack-source behavior separate from dynamic source handling, and keep
`PointerBasePlusOffset` as address materialization rather than a source memory
access. A dynamic `StackSlot` source with pointer-base fields is not sufficient
authority until the source `LoadLocal` memory access is attached and validated.

## Proof

No build/test proof required for this audit-only packet. No code changed;
updated `todo.md` only.
