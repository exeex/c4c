# BIR Route1 Scalar Producer Body Extraction

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 3 of 13, after `ideas/open/521_bir_route8_return_chain_body_extraction.md`
Owning Layer: BIR route1 scalar producer analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route1 scalar producer implementation bodies from `src/backend/bir/bir.cpp`
as the shared route boundary for later route extractions.

## Why This Exists

Route1 producer identity and integer constant helpers are shared by routes 2,
4, 5, 6, 7, and 8. A focused implementation owner should make those dependencies
explicit before extracting the larger dependent route families.

## In Scope

- Move existing route1 body implementations only.
- Add `bir_route1.cpp` and a private helper header only if needed.
- Preserve public query declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route1 callers, callees, and
  type references before moving code.

## Out Of Scope

- Do not change scalar producer semantics, integer constant evaluation, or
  source identity matching.
- Do not move `Value` declarations or constructors.
- Do not move route declarations out of `bir.hpp`.
- Do not add new producer capability for idea 422.

## Acceptance Criteria

- Build proof passes.
- Focused backend proof covers producer-index consumers, including downstream
  route4, route5, route6, route7, and route8 paths where available.
- `git diff --check` passes.

## Closure Note

Closed after moving the route1 scalar producer implementation bodies and
route1-only private helpers from `src/backend/bir/bir.cpp` into
`src/backend/bir/bir_route1.cpp`. Public route1 declarations and public BIR
types remain in `src/backend/bir/bir.hpp`; comparison-only helpers and
downstream route bodies remain outside the route1 translation unit.

Accepted proof covered `git diff --check`, a fresh default build, and the
backend CTest subset with `345/345` passing. The close-time regression guard
passed against the rolled-forward backend log with no new failures.

## Reviewer Reject Signals

- Producer identity, constant folding, or value matching changes.
- Public API churn exceeds what body relocation requires.
- Route1 extraction is combined with route2, route4, route6, or route7 behavior
  changes.
- Named-case shortcuts are added.
