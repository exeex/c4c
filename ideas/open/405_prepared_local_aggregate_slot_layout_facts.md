# Prepared Local Aggregate Slot Layout Facts

Status: Open
Type: Follow-up producer repair idea
Parent: `ideas/closed/400_rv64_object_route_local_memory_addressing_edges.md`
Discovered From: `tests/c/external/gcc_torture/src/20020225-2.c`

## Goal

Repair prepared producer publication for local aggregate and union storage whose
frame-slot facts contradict the byte width and range of the actual local memory
accesses consumed by RV64 object emission.

## Why This Exists

Idea 400's valid F64 local-memory lowering now rejects inconsistent prepared
facts instead of compensating in the RV64 object emitter. The inherited
`src/20020225-2.c` representative reaches local-memory lowering after earlier
floating-cast work, but prepared facts still publish one-byte union slots while
the object route observes 8-byte and 4-byte local accesses. The selected slot
therefore cannot contain the requested access size.

That is a producer/layout boundary: the prepared contract must expose coherent
slot size, offset, alignment, alias, and range facts for local aggregate or
union storage before target emission may lower it.

## In Scope

- Classify local aggregate and union storage cases where prepared frame-slot
  metadata is narrower than the emitted local memory access.
- Repair the producer-side slot layout or range publication so prepared facts
  describe the actual storage extent consumed by object emission.
- Preserve enough structured ownership to distinguish valid byte-field access
  from wider whole-object, member, or union-overlay access.
- Prove `src/20020225-2.c` and nearby local aggregate/union slot cases through
  the backend route once facts are coherent.

## Out Of Scope

- Teaching RV64 object emission to infer C union layout, aggregate aliasing, or
  source-level storage extent from instruction shape.
- Reopening scalar or floating-cast lowering already handled by earlier ideas.
- Broad ABI stack-frame or parameter-home admission work owned by
  `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`.
- Expectation rewrites, unsupported downgrades, or allowlist-only progress.

## Acceptance

- `src/20020225-2.c` no longer fails first because an 8-byte or 4-byte local
  memory access is matched to a one-byte prepared union slot.
- Prepared dumps show internally consistent local aggregate/union slot size,
  offset, alignment, and range facts for the repaired access family.
- RV64 object emission consumes explicit prepared facts and either lowers the
  case or fails at a distinct later boundary.
- Backend proof covers the representative plus at least one nearby same-family
  local aggregate or union slot case when available.

## Reviewer Reject Signals

- Reject filename-specific handling for `20020225-2.c` or any named union
  field.
- Reject RV64 emitter changes that fabricate slot size, source union layout,
  aliasing, or containment facts not present in prepared metadata.
- Reject merely widening all one-byte slots without preserving member offsets,
  overlay semantics, access width, and range correctness.
- Reject expectation downgrades, unsupported markers, or allowlist filtering
  claimed as producer progress.
- Reject a renamed diagnostic if the same contradictory one-byte slot fact still
  blocks the representative.
