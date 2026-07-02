# BIR Route4 Publication Body Extraction

Status: Open
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 5 of 13, after `ideas/open/523_bir_route2_select_chain_body_extraction.md`
Owning Layer: BIR route4 publication availability
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route4 current-block and block-entry publication body implementations
from `bir.cpp` before route5 and route6 cleanup.

## Why This Exists

Route4 is a cohesive publication availability family consumed by route-index
validation and route6 helpers. It should become a clear owner before route5 and
route6 are split.

## In Scope

- Move existing route4 implementation bodies only.
- Add `bir_route4_publication.cpp` and private helper declarations only if
  needed.
- Preserve public route4 declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route4 callers, callees, and
  facade/route6 consumers.

## Out Of Scope

- Do not move route4 public declarations.
- Do not move route-index facade or route-specific validation records.
- Do not alter route6 publication source selection.
- Do not combine route4 and route5 extraction.

## Acceptance Criteria

- Build proof passes.
- Focused publication proof covers current-block and block-entry publication
  availability.
- Route6 publication coverage passes when route6 consumes moved route4 bodies.

## Reviewer Reject Signals

- Route-index facade or validation behavior changes.
- Publication availability records change shape, order, or optionality.
- Route6 gains private implementation coupling to route4.
- Expectation rewrites substitute for unchanged-behavior proof.
