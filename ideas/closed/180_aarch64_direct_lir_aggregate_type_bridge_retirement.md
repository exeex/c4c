# AArch64 Direct LIR Aggregate Type Bridge Retirement

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/174_aggregate_abi_classification_structured_facts.md`
- `ideas/closed/176_lir_type_ref_structured_equality.md`

## Goal

Retire or fence the AArch64 direct-LIR aggregate type text bridge for one
bounded ABI/layout route.

The completed aggregate ABI work preserved older or mirrorless paths that may
still use named legacy fallbacks. This idea targets the AArch64/direct-LIR
surface where aggregate type information is still likely to arrive as rendered
type text instead of structured `LirTypeRef` / layout metadata.

## Why This Idea Exists

Idea 174 deliberately avoided retiring every AArch64 direct-LIR text parser.
Idea 176 made structured `LirTypeRef` equality meaningful when ids exist. The
next step is to stop treating direct-LIR aggregate text as ordinary authority
for one AArch64 path that can now provide structured facts.

## In Scope

- Inventory AArch64 direct-LIR aggregate type parser/fallback usage.
- Select one route where structured aggregate type metadata can be required or
  propagated.
- Prefer `LirTypeRef` structured ids and aggregate layout facts over rendered
  type text for metadata-rich inputs.
- Keep raw direct-LIR text compatibility only for explicit no-id legacy
  fixtures.
- Add focused tests showing metadata-rich AArch64 aggregate handling rejects
  stale or missing structured type facts instead of reparsing text.

## Out Of Scope

- Retiring every direct-LIR parser in one slice.
- Changing ABI printer syntax or final assembly output.
- Rewriting non-AArch64 backend ABI paths.
- Treating hand-authored no-id LIR fixtures as generated metadata-rich inputs.

## Acceptance Criteria

- One AArch64/direct-LIR aggregate route no longer uses rendered type text as
  normal semantic authority.
- Legacy no-id direct-LIR compatibility remains explicit.
- Tests cover structured success and stale/missing structured metadata.
- Validation includes targeted AArch64/direct-LIR or backend ABI coverage.

## Completion Notes

Closed after the selected AArch64 direct-LIR function signature ABI route
stopped using rendered aggregate type text as normal semantic authority for
metadata-rich aggregate returns and byval parameters. The route now resolves
aggregate layout through structured `LirTypeRef` / `StructNameId` facts, fails
closed for stale, missing, opaque, or no-ID metadata-rich AArch64 signature
facts, and keeps legacy no-id compatibility fenced to explicit no-signature or
non-selected compatibility paths.

Focused coverage proves structured success, stale or missing metadata
rejection, return and byval enforcement, and retained no-id compatibility
boundaries. The close review reported no blocking findings and no testcase
overfit. The close gate used matching canonical focused logs with 3/3 before
and 3/3 after and no new failures; broader `^backend_` validation also passed
109 executed tests with only the expected disabled MIR CLI tests skipped.

## Reviewer Reject Signals

- The fix adds a new rendered type parser branch.
- Metadata-rich aggregate inputs silently fall back to rendered text.
- Tests only check printer text or assembly spelling.
- The slice broadens into unrelated target ABI rewrites.
