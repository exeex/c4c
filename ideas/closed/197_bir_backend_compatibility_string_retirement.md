# BIR Backend Compatibility String Retirement

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/closed/196_hir_pending_consteval_structured_identity.md`

## Closure Notes

Closed after Step 6 recorded the compatibility-retirement ledger in `todo.md`
and the full-suite before/after proof logs matched with no regressions:
3137 passed, 0 failed, with the same 12 disabled backend CLI trace/focus tests
not run.

The closed interface contract is that new backend restart work may consume the
post-retirement BIR/prepared surfaces without adding rendered-name recovery
fallbacks for covered metadata-rich text, link-visible symbol, and type/layout
identity. Consumers that still lack a structured carrier should stay behind the
existing named no-id compatibility boundary or create a separate upstream
carrier initiative.

## Goal

Retire the remaining BIR and backend-prepared compatibility string authority
that is still intentionally carried after the frontend-to-BIR lookup closure
gate, so target backend restart work can consume structured identity rather
than rediscovering semantics from display spelling.

## Why This Idea Exists

The current BIR model already labels many retained strings as display,
diagnostic, final output spelling, route-local handles, or no-id compatibility.
That is useful containment, but it also means both the structured path and the
legacy compatibility path still exist in selected interfaces.

After ideas 195 and 196 close the remaining frontend-to-BIR structured identity
gates, this idea should remove or hard-fence the BIR-side compatibility holes
that would otherwise become accidental semantic dependencies for a new
`BIR -> MIR`, `PreparedBirModule -> MIR`, or AArch64 backend route.

## In Scope

- Inventory BIR fields and lowering/prealloc helpers that still retain raw
  string compatibility next to structured ids, including:
  - `bir::StringConstant::name`
  - structured type spelling and aggregate layout lookup bridges
  - `GlobalTypes`, `TypeDeclMap`, and related LIR-boundary compatibility maps
  - backend-prepared route-local name lookups that could be mistaken for
    semantic identity
- Convert pure text lookup to `TextId` where the value is a text-pool/string
  constant identity rather than final output spelling.
- Convert link-visible symbol lookup to `LinkNameId` where the value denotes a
  global, function, or pointer-to-symbol identity.
- Keep route-local SSA, slot, block, value, and debug focus spellings only when
  they are explicitly route-local and cannot cross a semantic boundary.
- Add focused tests that prove metadata-rich paths fail closed on stale or
  missing ids instead of recovering through matching raw strings.
- Produce a closure ledger stating which retained strings are final spelling,
  diagnostics, route-local handles, or explicit raw/no-id compatibility.

## Out Of Scope

- Starting or completing the AArch64 backend implementation.
- Removing strings required by printers, diagnostics, assemblers, linkers, or
  final object/output spelling.
- Rewriting the complete aggregate layout system in one pass.
- Changing parser, sema, HIR, or LIR source intent except where a missing
  upstream carrier must be recorded as a separate follow-up idea.
- Weakening supported behavior or changing tests to hide compatibility gaps.

## Acceptance Criteria

- BIR string constants and text-pool identities no longer require raw string
  lookup on metadata-rich generated paths, or the remaining raw path is fenced
  as explicit no-id compatibility with an owner and removal condition.
- Link-visible global/function identity consumed by BIR, prealloc, or target
  MIR preparation uses `LinkNameId` authority when metadata is present.
- Structured type/layout compatibility maps are either converted to structured
  ids or fenced so target backend work cannot treat rendered type spelling as
  ordinary semantic authority.
- Focused tests cover structured success, stale-id fail-closed behavior, and
  any intentionally retained no-id compatibility path.
- The final ledger states whether new backend restart work may consume the
  post-retirement BIR/prepared interface without adding new string-recovery
  fallbacks.

## Reviewer Reject Signals

- The route claims progress by renaming compatibility helpers while preserving
  the same raw string lookup authority on metadata-rich paths.
- The implementation adds new named-test or testcase-shaped string matching to
  make a narrow backend case pass.
- A supported path is downgraded, marked unsupported, or given weaker expected
  output without explicit user approval.
- A `LinkNameId`, `TextId`, `SlotNameId`, or `BlockLabelId` mismatch can still
  recover through identical rendered spelling on a covered metadata-rich path.
- Printer, assembler, linker, or debug-focus spelling is incorrectly treated as
  semantic identity instead of final output or observation text.
- Broad backend rewrite work begins before the compatibility retirement ledger
  and fail-closed tests are complete.
