# BIR Route2 Select-Chain Body Extraction

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 4 of 13, after `ideas/open/522_bir_route1_scalar_producer_body_extraction.md`
Owning Layer: BIR route2 select-chain analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route2 select-chain and direct-global dependency bodies while recording
route6 as a stable consumer of the public route2 surface.

## Why This Exists

Idea 518 identified route2 as cohesive but consumed by route6 call-publication
logic. The cleanup should clarify the route2 owner without hiding direct-global
dependency behavior inside route6.

## In Scope

- Move existing route2 implementation bodies only.
- Add `bir_route2.cpp` and private helper declarations only if needed.
- Preserve public route2 declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route2 symbols and route6
  consumers before moving code.

## Out Of Scope

- Do not change direct-global dependency classification.
- Do not edit route6 publication policy, call ABI behavior, or idea 422
  producer behavior.
- Do not fold route2 helpers into route6.
- Do not split public headers in this slice.

## Acceptance Criteria

- Build proof passes.
- Focused select-chain proof passes.
- Route6 call-publication coverage that consumes route2 direct-global facts
  passes.

## Reviewer Reject Signals

- Route6 reaches into private route2 implementation details.
- Direct-global select-chain availability changes.
- Header extraction appears in this body-only slice.
- Tests or expectations are weakened.

## Closure Note

Closed after extracting route2 select-chain value-record bodies and
`route2_find_direct_global_dependency` into
`src/backend/bir/bir_route2.cpp`. Public route2 declarations and record types
remain in `bir.hpp`, and route6 continues to consume the public
`route2_select_chain_value_record` surface rather than route2-private details.

Accepted proof covered `git diff --check`, default build, backend subset
validation with `345/345` tests, and close-time regression guard against the
rolled-forward `test_before.log` with `345/345` before and after and no new
failures.
