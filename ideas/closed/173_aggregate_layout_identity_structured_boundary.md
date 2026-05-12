# Aggregate Layout Identity Structured Boundary

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/172_type_identity_authority_audit.md`

## Goal

Move aggregate layout identity across the HIR -> LIR -> BIR/backend boundary
away from rendered struct/type spelling and toward structured record and layout
identity that can reject collisions or stale compatibility data.

## Why This Idea Exists

The type identity audit found aggregate layout identity as the highest-risk
remaining gap. HIR already has record definitions, owner keys, owner indexes,
and computed struct layout, while downstream LIR/BIR/backend surfaces still
cross rendered bridges such as `base_tags`, `struct_defs`, `type_decls`,
final `%struct...` spelling, and recursive aggregate field text. A stale or
colliding spelling can affect size, alignment, GEPs, aggregate copies, globals,
and stack layout even when stronger record identity exists upstream.

## In Scope

- Identify the structured authority for generated aggregate layout data:
  HIR record owner identity, record definitions, field metadata, and computed
  layout facts.
- Carry enough structured layout identity through generated LIR and BIR
  surfaces that metadata-rich paths do not rely on rendered struct names for
  layout lookup, mirror checks, or field offset decisions.
- Shrink compatibility bridges around rendered `struct_defs`, `base_tags`,
  LIR `type_decls`, and backend structured layout maps.
- Make metadata-rich mismatches fail closed instead of falling back to an equal
  rendered spelling.
- Prove the slice with build proof plus focused layout/backend coverage such as
  HIR record layout cases and aggregate member-offset/global aggregate routes.

## Out Of Scope

- Rewriting the full HIR, LIR, or BIR type system.
- Removing every legacy rendered layout bridge in one slice.
- Changing final LLVM type spelling or diagnostic display names just because
  they are strings.
- Adding a new parser for rendered `%struct...` names as the main fix.

## Acceptance Criteria

- Generated aggregate layout decisions have a structured identity path from HIR
  layout facts into the relevant LIR/BIR/backend consumer touched by the slice.
- The changed consumer no longer treats equal rendered aggregate spelling as
  sufficient identity when structured metadata is present.
- Compatibility behavior remains isolated for legacy/no-metadata inputs and is
  explicitly separated from generated metadata-rich inputs.
- Focused layout or backend route tests still assert meaningful layout, offset,
  copy, global, or stack behavior without weakening expectations.
- The proof includes a fresh build or compile check plus targeted CTest or
  route coverage for the changed boundary.

## Closure Summary

Closed after completing the selected local aggregate GEP layout boundary.
Metadata-rich generated local aggregate GEP inputs now use structured layout
identity and fail closed when the structured context is stale, mismatched,
opaque, or otherwise unusable for that consumer. Equal rendered aggregate
spelling is no longer sufficient to accept this metadata-rich local GEP
boundary, while legacy/no-metadata compatibility remains isolated behind the
existing explicit fallback.

Focused proof covered `backend_prepare_structured_context` and
`backend_lir_to_bir_notes`; the canonical regression guard passed for matching
`test_before.log` and `test_after.log` focused logs. Broader backend validation
passed, and the accepted full-suite baseline for commit
`9aed9b5d0fbbb348ee25819ee2a715c0a5aa5793` passed 3137 tests with no failures.

Follow-up aggregate identity work for globals, byval copies, call ABI,
stack/global layout routes, or broader backend aggregate migration remains
outside this selected boundary and should be captured as separate ideas when
needed.

## Reviewer Reject Signals

- The route adds another rendered struct-name parser branch and claims that as
  structured layout identity.
- A generated metadata-rich layout miss silently falls back to text-keyed
  `type_decls`, `%struct...` names, or recursive field text.
- Existing layout/backend tests are downgraded, marked unsupported, or changed
  to accept weaker offset, size, align, copy, or global behavior without
  explicit user approval.
- The diff mainly renames helper functions or rewrites expectations while the
  same spelling-keyed layout authority still decides field offsets or ABI
  layout.
- The slice expands into broad HIR/LIR/BIR rewrites without a focused layout
  boundary and focused proof route.
