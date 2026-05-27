Status: Active
Source Idea Path: ideas/open/45_shared_prepared_aggregate_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Aggregate Stack-Source Authority

# Current Packet

## Just Finished

Step 1 - Audit Aggregate Stack-Source Authority completed.

Observed aggregate `StackSlot -> Register` source shapes:

- Concrete stack-home aggregate shape: prepared edge publication can carry a
  `PreparedValueHomeKind::StackSlot` source with `slot_id`, concrete
  `offset_bytes`, `size_bytes` greater than scalar width, optional
  `align_bytes`, destination `Register` home, and an OutOfSsa parallel-copy
  `PreparedMoveResolution`. The RISC-V focused test covers signed-12 and large
  offsets with `size_bytes == 16` and proves they stay
  `UnsupportedSourceHome`.
- Dynamic LoadLocal-backed stack-source shape: shared lookup can identify
  `source_producer_kind == LoadLocal` and, when addressing is prepared, expose
  `source_memory_*` facts: base kind/value, byte offset, size, alignment,
  address space, volatility, base-plus-offset support, and materialization
  need. Current supported use is scalar same-width I32 only; this producer
  shape does not yet describe aggregate lanes or ABI layout.
- Stack-to-stack neighbors exist in the same edge-publication consumer, but
  aggregate stack sources to stack destinations are also fail-closed and should
  not be used as authority for this `StackSlot -> Register` packet.

Currently available shared facts:

- `PreparedValueHome` preserves storage kind, value id/name, register spelling,
  target register identity, frame slot id, stack offset, size, alignment,
  immediate payloads, and pointer-base-plus-delta decorations.
- `PreparedEdgePublication` preserves edge predecessor/successor,
  source/destination BIR values and value ids, source/destination homes and
  home kinds, destination storage kind, matching move/bundle, parallel-copy
  metadata, producer kind, and optional prepared memory-access facts for
  LoadLocal producers.
- `prepare_same_width_i32_stack_source_publication` is explicitly scalar:
  it requires concrete `StackSlot` source, `size_bytes == 4`, I32 source and
  destination types, register destination storage, OutOfSsa move authority,
  GPR destination placement, and records `SameWidthNoExtension`.

Scalar paths that must remain distinct:

- RISC-V same-width I32 concrete stack source to GPR, including large offsets,
  goes through `PreparedTypedStackSourcePublication` and destination placement.
- RISC-V existing concrete I64 stack source to register is a narrow preserved
  scalar path using source/destination I64 type plus `size_bytes == 8`.
- RISC-V dynamic LoadLocal stack source is scalar I32 only and requires complete
  shared source-memory access facts; missing or incomplete access remains
  fail-closed.
- RISC-V pointer-base materialization and I32 stack-destination copies are
  separate publication forms, not aggregate source-copy authority.
- AArch64 scalar value-home publication uses scalar type views and scalar load
  width, and x86 prepared edge publication renders a DWORD stack operand from
  offset only; neither is aggregate authority.

Unsafe inference routes:

- Do not infer aggregate copy behavior from byte size alone, including treating
  `size_bytes == 8` or `16` as one scalar load/store, or selecting lane loads
  from the raw byte count.
- Do not infer destination lanes from physical register spelling, fixture
  destination names, value ids, stack slot ids, stack offsets, or
  `PreparedRegisterPlacement::contiguous_width` alone.
- Do not infer aggregate source bytes from LoadLocal shape, pointer-base
  decorations, or `source_memory_size_bytes` without an aggregate copy contract.
- Do not reuse AArch64 scalar home publication or x86 DWORD operand rendering
  as proof that aggregate edge-publication semantics exist.

No aggregate family is selected for implementation yet. The precise upstream
producer gap is the absence of a target-neutral aggregate stack-source
publication fact carrying copy width, source byte ranges/base class, destination
register or lane mapping, lane widths/byte offsets, minimum alignment,
partial-copy validity, ABI layout reference, and scratch ownership
expectations. Step 2 should add an explicit shared aggregate authority record
or fail-closed lookup status for one concrete family only when these facts are
available from prepare; otherwise it should make this gap observable without
letting targets rediscover the copy locally.

## Suggested Next

Step 2 of `plan.md`: add a shared prepared aggregate authority carrier or
explicit fail-closed status for the concrete aggregate `StackSlot -> Register`
family, starting from the producer-gap fields above rather than target-local
byte-size lowering.

## Watchouts

Preserve idea 42 fail-closed RISC-V coverage. The x86 prepared edge consumer is
especially risky because it currently accepts any stack slot with an offset as
`DWORD PTR [rsp + offset]`; that must not be treated as aggregate support.
Keep scalar I32/I64 and dynamic I32 paths separate from any new aggregate
record.

## Proof

Audit-only packet; no build or test proof was required, no broad tests were
run, and no `test_after.log` was produced.
