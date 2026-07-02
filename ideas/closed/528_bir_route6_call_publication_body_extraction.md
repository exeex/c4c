# BIR Route6 Call Publication Body Extraction

Status: Closed
Type: Behavior-preserving cleanup
Parent: `ideas/closed/518_bir_core_model_cleanup_umbrella.md`
Order: BIR cleanup follow-up 9 of 13, after `ideas/open/527_bir_route7_comparison_body_extraction.md`
Owning Layer: BIR route6 call publication analysis
Source Artifact: `docs/bir_core_cleanup/follow_up_ideas.md`

## Goal

Extract route6 call-use, call-argument publication, call-result source, and
publication-routing bodies only after route1 through route5 are stable.

## Why This Exists

Route6 composes route1-route5 and call ABI facts. It is high-risk and should
move late as a behavior-preserving owner extraction, not as a semantic rewrite.

## In Scope

- Move existing route6 implementation bodies only.
- Add `bir_route6_call_publication.cpp` and private helper declarations only if
  needed.
- Preserve route6 public declarations in `bir.hpp`.
- Use `.codex/skills/c4c-clang-tools/` to map route6 dependencies and direct
  callers/callees before moving code.

## Out Of Scope

- Do not change call ABI lowering or LIR-to-BIR call generation.
- Do not implement idea 422 producer capability.
- Do not move route6 declarations out of `bir.hpp`.
- Do not move route-index facade or memory provenance headers.

## Acceptance Criteria

- Build proof passes.
- Focused call-publication, call-result, and publication-routing proof passes.
- Supervisor considers a full `^backend_` subset if narrower proof is not
  enough for this composed route.

## Completion Note

Closed after the route6 call-use, call-argument publication, call-result
source, and publication-routing bodies were extracted into
`src/backend/bir/bir_route6_call_publication.cpp` without moving route6 public
declarations, route-index facade bodies, memory provenance surfaces, call ABI
lowering, LIR-to-BIR call generation, or route1-route5 owner behavior.

Accepted proof covered backend build plus `^backend_`, then focused route6
checks for `backend_lir_to_bir_notes`, `backend_prepared_lookup_helper`,
`backend_prealloc_call_boundary_classification`, and
`backend_aarch64_call_boundary_owner`. Close-time regression guard reused the
focused four-test baseline and refreshed matching `test_after.log` with 4/4
passing before and after.

## Reviewer Reject Signals

- Call-publication source selection, call-result identity, or route dependency
  behavior changes.
- Route6 duplicates route1-route5 logic instead of using stable APIs.
- The slice expands into call ABI lowering or idea 422 producer work.
- Header extraction or facade movement appears in the same patch.
