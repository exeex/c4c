# Prepared Stack-Slot Preservation Source Publication

Status: Open
Type: Implementation
Parent: `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Related:
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Owning Layer: prepared call-boundary authority production
Queue Order: 24
Prerequisites: ordinary call preservation planning must identify live values that must survive a call and their concrete source/destination endpoints
Estimated Evidence Breadth: current blocker includes `src/20020529-1.c`; refresh diagnostics before implementation
Proof Surface: prepared call preserve facts, stack-slot preserve source endpoints, and RV64 call ABI consumer fail-closed guards

## Goal

Publish explicit concrete source endpoints for prepared stack-slot preserves
that keep call-live values across ordinary same-module calls, so RV64 call ABI
consumers can lower the preserves without inferring source registers from
parameter position, storage summaries, or final assembly shape.

## Why This Exists

Idea 613 is an ABI/RV64 consumer route. During Step 2 classification,
`src/20020529-1.c` still stopped at `unsupported_call_abi` because the `%p.b`
stack-slot preserve carried a concrete destination slot but only published
`preservation_source=register:value#1`, with no concrete source register. The
function storage summary still showed the parameter in `a1`, but consuming that
summary as callsite preserve authority would cross from consumer lowering into
prepared authority production.

## In Scope

- Produce explicit concrete source register or source-location facts for
  stack-slot preserves created by ordinary call-boundary preparation.
- Preserve the value identity, destination stack slot, size/width, and
  callsite association needed by downstream RV64 consumers.
- Keep diagnostics fail-closed when the preserve source endpoint is missing,
  ambiguous, stale, or only derivable from unrelated storage summaries.
- Reclassify `src/20020529-1.c` if refreshed diagnostics prove a more precise
  producer-layer blocker owns the row.

## Out Of Scope

- RV64 object-emission lowering of completed preserve facts.
- Inferring preserve source registers inside RV64 from ABI parameter position,
  storage summaries, testcase names, or final assembly shape.
- Outgoing stack argument destination offsets covered by idea 624.
- Variadic or library call policy.
- Runtime mismatch triage.
- Local/global producer repairs, stack-frame consumer repairs, expectation
  changes, unsupported marker changes, allowlists, timeouts, or accounting.

## Acceptance Criteria

- At least one ordinary same-module stack-slot preserve row exposes a concrete
  prepared source endpoint before RV64 object emission.
- `src/20020529-1.c` either moves past the missing preserve-source blocker or
  is reclassified with a more precise non-producer blocker backed by current
  diagnostics.
- RV64 consumers can require explicit preserve source endpoints instead of
  deriving them from parameter position or function storage state.
- Negative proof keeps missing, ambiguous, stale, and summary-only preserve
  sources rejected.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts that special-case `src/20020529-1.c` or
  any named torture row.
- Reject RV64 consumer code that infers stack-slot preserve sources from ABI
  parameter position, storage summaries, final assembly layout, or testcase
  names instead of consuming prepared facts.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or classification-only changes that leave stack-slot
  preserve facts without explicit concrete source endpoints.
- Reject broad ABI, variadic, library, local/global producer, or runtime
  rewrites that are not needed to publish stack-slot preserve source authority.
