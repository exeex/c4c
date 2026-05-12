# BIR Direct Symbol Identity Validation Closure

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/162_linknameid_backend_symbol_authority.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`

## Goal

Close the BIR direct symbol identity validation boundary so direct calls,
global loads/stores, and pointer initializers use `LinkNameId` as semantic
identity whenever metadata is available.

Raw callee/global strings may remain for display, diagnostics, runtime
placeholder calls, and explicit compatibility inputs, but generated
metadata-rich paths should not use them as identity.

## Why This Idea Exists

BIR now carries `LinkNameId` for functions and globals, but several data
structures still retain both final spelling and id fields. The current
validator already checks many pairings, yet backend restart would benefit from
a closure pass that proves the direct-symbol boundary is fail-closed and not
string-first.

## In Scope

- Audit BIR `CallInst`, `LoadGlobalInst`, `StoreGlobalInst`, global
  declarations, function declarations, and pointer initializer symbol
  handling.
- Strengthen validation for mismatched or missing `LinkNameId` on generated
  metadata-rich direct symbol references.
- Preserve explicit compatibility/display uses of callee/global strings.
- Ensure runtime/intrinsic placeholder calls remain intentionally invalid-id
  paths rather than ordinary user/extern symbol identity.
- Add focused tests for mismatched id/name pairs, missing id metadata, and
  retained placeholder compatibility.

## Out Of Scope

- Changing object-file symbol spelling or assembler string tables.
- Replacing every local SSA, slot, or block label string.
- Rewriting LinkNameTable itself.
- Retiring no-metadata raw BIR/LIR imports in this slice.

## Acceptance Criteria

- Metadata-rich direct symbol references validate through `LinkNameId`.
- String-only direct symbol identity is fenced to explicit compatibility or
  placeholder cases.
- Tests cover direct calls and at least one global or pointer-initializer
  symbol reference.
- Validation includes focused BIR validation/lowering coverage.

## Reviewer Reject Signals

- The implementation trusts matching final strings even when structured ids
  disagree.
- Invalid `LinkNameId` is accepted for generated user/extern direct symbols
  without a compatibility reason.
- Tests weaken existing expectations or only check printed names.
- Placeholder/runtime calls are conflated with user/extern function identity.
