# BIR Function Signature Byval Metadata Text Retirement

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`

## Goal

Retire the remaining metadata-rich BIR function-signature path that parses
`signature_text` to decide explicit byval aggregate parameter handling.

Generated function signatures should expose byval/aggregate parameter facts
directly through structured signature metadata rather than requiring a second
parse of the final LLVM header spelling.

## Why This Idea Exists

The second frontend-to-BIR audit found that aggregate parameter collection can
have structured signature params available while still parsing
`function_.signature_text` to detect explicit byval syntax. That is a narrow
but important remaining string lookup path because it can affect ABI/layout
decisions.

## In Scope

- Inspect `BirFunctionLowerer::collect_aggregate_params` and related function
  signature metadata producers.
- Add or reuse a structured explicit-byval marker for function signature
  parameters.
- Make metadata-rich aggregate parameter collection fail closed if structured
  byval metadata is stale or missing.
- Preserve `signature_text` parsing only for legacy hand-built/no-metadata LIR
  fixtures.
- Add tests where stale `signature_text` byval spelling cannot override
  structured metadata.

## Out Of Scope

- Full call ABI redesign.
- Removing `signature_text` as final output spelling.
- Changing raw legacy LIR fixture behavior except to fence it explicitly.
- Reworking unrelated aggregate local-slot lowering.

## Acceptance Criteria

- Metadata-rich generated function signature byval handling does not parse
  `signature_text` for semantic decisions.
- Structured byval metadata drives aggregate parameter layout and ABI facts.
- Legacy no-metadata fallback is explicit and tested.
- Focused BIR aggregate/function-signature tests cover stale text rejection.

## Completion Notes

- `LirSignatureParam::is_byval` is now the structured signature metadata
  carrier for explicit incoming byval aggregate parameters.
- Generated HIR-to-LIR signatures populate the structured byval fact alongside
  signature parameter type refs, and verification rejects inconsistent
  metadata-rich mirror state instead of recovering from rendered text.
- BIR aggregate parameter collection consumes structured signature metadata
  when available, so generated metadata-rich lowering no longer parses
  `signature_text` to decide explicit byval handling.
- Legacy `signature_text` parsing remains only for raw/no-metadata LIR fixture
  compatibility, with tests covering that boundary.
- Focused frontend/backend coverage proves stale rendered signature text cannot
  override structured metadata, and close-time full-suite regression guard
  passed with `3137/3137` runnable tests green before and after.

## Reviewer Reject Signals

- The implementation still consults `signature_text` when structured signature
  metadata is available.
- Missing structured byval metadata silently falls back to rendered header text.
- Tests only cover normal matching text and metadata.
- The slice changes final printed function headers unnecessarily.
