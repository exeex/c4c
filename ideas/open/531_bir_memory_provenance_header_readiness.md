# BIR Memory Provenance Header Readiness

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 12 of 13, after `ideas/open/530_bir_route_header_split_after_body_moves.md`
Owning Layer: BIR memory provenance declaration surface
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Consider a public memory provenance support header only after route3 and
LIR-to-BIR memory consumers are mapped.

## Why This Exists

Memory provenance and storage authority are public BIR model surfaces, not
private lowering helpers. They may deserve a narrower public header, but only
after include consumers and route3 ownership are stable.

## In Scope

- Audit memory provenance, storage authority, static GEP, dynamic-array, and
  route3/lowering consumers.
- Move declarations only if a public support header reduces coupling without
  changing semantics.
- Keep behavior, storage layout, enum values, and verdicts identical.
- Use `.codex/skills/c4c-clang-tools/` type-reference and caller/callee
  queries before selecting boundaries.

## Out Of Scope

- Do not move memory provenance declarations into `lir_to_bir/memory/`.
- Do not change pointer-value provenance, static GEP authority, object extent,
  byte-range, dynamic-array, or storage authority semantics.
- Do not edit idea 422 producer behavior.
- Do not move core `MemoryAddress` usage unless a separate core-header split
  owns that work.

## Acceptance Criteria

- Build proof passes.
- Focused route3 memory access, LIR-to-BIR memory lowering, object emission,
  and pointer-value provenance proof passes.
- Supervisor prefers broader backend validation if public model support
  surfaces move.

## Reviewer Reject Signals

- Public model consumers depend on private lowering headers.
- Any provenance authority verdict changes.
- Declaration movement is combined with new memory behavior.
- `Function` or instruction storage layout changes incidentally.
