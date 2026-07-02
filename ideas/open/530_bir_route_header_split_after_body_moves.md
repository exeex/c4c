# BIR Route Header Split After Body Moves

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 11 of 13, after `ideas/open/529_bir_route_facade_body_extraction.md`
Owning Layer: BIR route declaration surface
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Split route declarations into narrow headers only after the route body owners
are stable.

## Why This Exists

Idea 518 rejected an early one-shot `bir.hpp` breakup. Header movement should
come after body extraction proves route dependency direction and include
pressure.

## In Scope

- Move route declarations out of `bir.hpp` only where narrower headers reduce
  coupling.
- Update include sites required by compile proof.
- Preserve public names, signatures, namespaces, enum values, and behavior.
- Use `.codex/skills/c4c-clang-tools/` plus include/build evidence before
  selecting header boundaries.

## Out Of Scope

- Do not move implementation bodies in this header-focused slice.
- Do not create one large replacement monolith unless review proves it reduces
  include pressure.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not change route semantics or public query names.

## Acceptance Criteria

- Build proof passes for all backend targets that include `bir.hpp`.
- Focused backend route tests pass after include churn.
- Header split reduces or clarifies dependencies without forcing broad
  consumers to include many new route headers.

## Reviewer Reject Signals

- Header split increases coupling or creates a new catch-all route monolith.
- Complete-type requirements for `Function` vectors break or rely on fragile
  forward declarations.
- Public API names, namespaces, or signatures change.
- Header work is mixed with semantic or body movement.
