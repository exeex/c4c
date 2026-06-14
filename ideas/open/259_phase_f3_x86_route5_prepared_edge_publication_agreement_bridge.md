# 259 Phase F3 x86 Route 5 prepared edge publication agreement bridge

## Idea Type

x86 backend agreement bridge.

## Goal

Add the narrow x86 bridge needed to compare Route 5 CFG-edge publication source
identity against the prepared `PreparedEdgePublication` lookup row used by the
x86 prepared edge-publication consumer, while keeping prepared lookup/status
and target emission policy as compatibility authority.

## Why This Exists

Idea 251 completed the Route 4/5 edge-publication blocker map and found that
adapter readiness is blocked only by x86. RISC-V already exposes diagnostic
Route 5 agreement fields, but x86 currently consumes prepared
edge-publication lookup/status data through
`consume_edge_publication_move_intent(...)` without a Route 5/BIR agreement
consumer or MIR query facade that joins the same-edge semantic record to the
prepared publication row.

## In Scope

- Introduce a narrow x86 Route 5/BIR agreement consumer or MIR query facade
  that joins the same predecessor, successor, destination value, source value,
  and source producer from `Route5CfgEdgePublicationRecord` /
  `BirCfgEdgePublicationSourceIdentity` to the prepared
  `PreparedEdgePublication` row used by x86.
- Fail closed for duplicate, mismatch, absent or wrong-edge, unsupported,
  prepared-only, and non-agreeing fallback rows.
- For dynamic `LoadLocal` publication sources, require Route 3 source-memory
  identity agreement before treating the Route 5 row as agreeing.
- Preserve public prepared `edge_publications` lookup helpers, status names,
  helper/oracle names, fallback rows, x86 dispatch/status rows, module output,
  and exact target output formatting.
- Add focused backend proof that covers the agreeing path and fail-closed
  rejection rows without weakening existing prepared expectations.

## Out Of Scope

- Deleting, privatizing, renaming, or bypassing public prepared
  `edge_publications` helpers.
- Moving move selection, register choice, carrier/helper selection, layout,
  instruction spelling, stack/source-home policy, or module-output formatting
  into Route 5/BIR.
- Rewriting riscv behavior, riscv diagnostic fields, or x86 output strings.
- Broad Route 4, Route 5, edge lowering, register allocation, or emission
  policy rewrites outside this x86 agreement bridge.

## Acceptance Criteria

- X86 has a concrete Route 5/BIR agreement consumer or MIR query facade that
  checks the same-edge prepared publication row before treating prepared
  edge-publication lookup as mirrored semantic agreement.
- Duplicate, mismatch, absent, unsupported, prepared-only, non-agreeing
  fallback, and dynamic `LoadLocal` Route 3 source-memory disagreement cases
  fail closed and preserve prepared compatibility output.
- Existing prepared edge-publication lookup/status behavior remains observable
  and stable.
- Focused backend tests prove the x86 agreement path and fail-closed rows
  without downgrading expectations.
- The implementation is narrow enough that prepared lookup/status and x86
  target emission policy remain compatibility-owned.

## Reviewer Reject Signals

- Reject named-case shortcuts that only special-case one edge label,
  publication kind, source name, predecessor/successor pair, or x86 output row
  while claiming a Route 5 agreement bridge.
- Reject unsupported expectation downgrades, weakened publication status
  contracts, or baseline rewrites without explicit approval.
- Reject helper renames, output rewrites, status-string rewrites, or
  classification-only changes claimed as Route 5 migration progress.
- Reject any route that lets prepared `edge_publications` remain the old
  semantic source behind a new bridge name instead of checking
  `Route5CfgEdgePublicationRecord` / `BirCfgEdgePublicationSourceIdentity`.
- Reject broad register allocation, carrier/helper, layout, instruction
  emission, Route 4, Route 5, or riscv rewrites outside this x86 agreement
  bridge.
- Reject an adapter that accepts duplicate, mismatch, absent, unsupported,
  prepared-only, non-agreeing fallback, or dynamic `LoadLocal` Route 3
  source-memory disagreement rows as semantic agreement.
