# BIR Route7 Comparison Body Extraction

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 8 of 13, after `ideas/open/526_bir_route5_publication_body_extraction.md`
Owning Layer: BIR route7 comparison analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route7 comparison record construction without moving facade-backed
public query helpers in the same slice.

## Why This Exists

Route7 comparison analysis is cohesive, but materialized-condition and fused
compare public queries are cross-route/facade consumers. The record
construction owner should be separated before facade cleanup.

## In Scope

- Move existing route7 record-construction bodies only.
- Add `bir_route7_comparison.cpp` and private helper declarations only if
  needed.
- Preserve public declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route7 symbols, facade users,
  and comparison type references.

## Out Of Scope

- Do not move route-index facade bodies.
- Do not move facade-backed materialized-condition or fused-compare public
  query helpers.
- Do not change comparison operand producer semantics.
- Do not add idea 422 producer behavior.

## Acceptance Criteria

- Build proof passes.
- Focused comparison condition indexing and materialized-condition consumer
  proof passes.
- Link-time backend proof passes because public query helpers remain in another
  translation unit.

## Reviewer Reject Signals

- Facade and route7 record construction move as one broad slice.
- Materialized-condition producer identity changes.
- Public/private conversion changes without a semantic source idea.
- Header splitting is mixed into body-only movement.
