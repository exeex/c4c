# BIR Route5 Publication Body Extraction

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 7 of 13, after `ideas/open/525_bir_route3_memory_access_body_extraction.md`
Owning Layer: BIR route5 CFG-edge and join-source publication
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route5 CFG-edge and join-source publication bodies after route3 and
route4 boundaries are stable.

## Why This Exists

Route5 has a distinct publication owner but depends on route1, route3, and
route4 concepts. It should be split only after those dependencies have clear
stable APIs.

## In Scope

- Move existing route5 implementation bodies only.
- Add `bir_route5_publication.cpp` and private helper declarations only if
  needed.
- Preserve public route5 declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route5 dependencies on route1,
  route3, route4, and route6 consumers.

## Out Of Scope

- Do not change route1 identity, route3 memory records, or route4 matching.
- Do not edit route6 publication policy.
- Do not combine route5 extraction with route6 extraction.
- Do not split route5 public declarations in this slice.

## Acceptance Criteria

- Build proof passes.
- Focused CFG-edge, join-source, and publication proof passes.
- Route6 coverage passes if route6 consumes route5 records in the linked
  subset.

## Reviewer Reject Signals

- Route3 or route4 matching logic is duplicated in route5.
- Join-source or CFG-edge record ordering/selection changes.
- Route5 extraction requires declaration movement or public API changes.
- Tests are rewritten to accept weaker publication contracts.
