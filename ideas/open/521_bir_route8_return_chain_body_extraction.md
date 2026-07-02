# BIR Route8 Return-Chain Body Extraction

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 2 of 13, after `ideas/open/520_bir_render_owner_preservation.md`
Owning Layer: BIR route8 return-chain analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route8 return-chain implementation bodies from `src/backend/bir/bir.cpp`
into a focused translation unit while leaving public declarations stable.

## Why This Exists

Idea 518 identified route8 as the most leaf-like route family, with route1
identity as its main shared input. It is a good first route body extraction
candidate after printer/render ownership is confirmed.

## In Scope

- Move existing route8 return-chain function bodies only.
- Add a focused route8 translation unit and build metadata if selected.
- Preserve existing public declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route8 symbols, definitions,
  callers, callees, and route1 dependencies before moving code.

## Out Of Scope

- Do not move route8 declarations out of `bir.hpp`.
- Do not alter `Route1SourceValueIdentity` or scalar producer behavior.
- Do not touch route6 call publication, route7 comparison, or facade helpers.
- Do not add BIR producer capability from idea 422.

## Acceptance Criteria

- Build proof passes.
- Focused return-chain/backend proof covers route8 public queries and callers.
- The same return-chain records, ordering, and optional/nullopt decisions are
  produced before and after the move.

## Reviewer Reject Signals

- Header splitting appears in the same slice as body extraction.
- Route8 semantics, ordering, optionality, or diagnostics change.
- Route8 gains direct coupling to route6 or facade internals.
- Expectation rewrites are used as proof of cleanup success.
