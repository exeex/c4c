# Byval Copy Layout Structured Boundary

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/173_aggregate_layout_identity_structured_boundary.md`
- `ideas/closed/174_aggregate_abi_classification_structured_facts.md`

## Goal

Make byval copy lowering use structured aggregate layout identity for one
bounded byval copy route.

Idea 174 structured the selected byval call-argument ABI classification path.
This idea targets the copy/materialization side: when a byval aggregate is
copied into local storage or decomposed into slots, the copy layout should come
from structured layout facts, not from rendered aggregate spelling.

## Why This Idea Exists

Byval handling has two adjacent responsibilities:

- ABI classification decides how the aggregate crosses the call boundary.
- Copy/materialization lowering decides how bytes, fields, or slots are moved.

The first has a structured slice from idea 174. The second can still be a
separate source of stale layout behavior if copy loops or slot materialization
trust text-shaped aggregate identity.

## In Scope

- Pick one byval copy/materialization family, such as parameter byval copy into
  BIR local slots or aggregate copy before prealloc.
- Thread structured aggregate owner/layout identity into that copy path.
- Use size/align/field facts from structured layout metadata for metadata-rich
  inputs.
- Fail closed or emit a clear unsupported metadata gap when structured layout
  facts are missing or mismatched for generated inputs.
- Preserve final output spelling and existing ABI classification behavior.
- Add focused byval copy tests with a stale or same-spelled aggregate layout
  collision.

## Out Of Scope

- Reworking all aggregate memcpy/memset routes.
- Reclassifying call ABI decisions already handled by idea 174.
- Replacing every rendered local slot or debug name.
- Weakening byval/backend route expectations.

## Acceptance Criteria

- The selected byval copy route consumes structured layout identity.
- Equal rendered spelling is not sufficient for metadata-rich byval copy
  layout.
- Focused tests prove the structured path and a mismatch/fail-closed case.
- Legacy/no-metadata compatibility is named and fenced.
- Validation includes targeted byval/backend route coverage.

## Completion Notes

The selected route was incoming byval parameter materialization into BIR local
slots. ID-bearing incoming byval parameters now use the structured type-ref
lookup path, fail closed on missing, stale, mismatched, opaque, invalid,
zero-sized, or zero-aligned structured metadata, and do not recover semantic
layout by reparsing rendered type text.

Focused notes-test coverage proves successful structured materialization from
the byval pointer, missing structured layout rejection, mismatched
`StructNameId` rejection, opaque declaration rejection, and the fenced legacy
no-ID compatibility path. The Step 3 route review found no testcase overfit,
expectation weakening, or route drift, and no additional implementation packet
was required before closure.

Close proof used the canonical focused before/after logs:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with both logs at 6 passed, 0 failed. Broader `^backend_`
validation also passed at `/tmp/c4c_backend_validation_byval_step3.log`.

## Reviewer Reject Signals

- The copy route still derives layout from rendered struct/type spelling first.
- The implementation hides missing structured metadata by reparsing text.
- Tests only check final return values or printed temps.
- The slice drifts into unrelated ABI or register-allocation work.
