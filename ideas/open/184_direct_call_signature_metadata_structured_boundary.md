# Direct Call Signature Metadata Structured Boundary

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/181_function_pointer_signature_type_identity.md`
- `ideas/open/183_lir_bir_backend_freeze_authority_audit.md`

## Goal

Bring direct-call signature lowering up to the same structured-metadata
standard established for the bounded indirect-call path.

Metadata-rich generated LIR direct calls should use structured call/function
signature facts for return type, parameter types, variadic state, byval, sret,
and aggregate layout. Rendered signature text may remain as compatibility or
display spelling, but not as normal semantic authority.

## Why This Idea Exists

Idea 181 closed the indirect function-pointer signature path, but its closure
explicitly left direct-call signature metadata as future scope. The current
LIR-to-BIR direct-call route still has legacy helpers that parse rendered
signature text for no-metadata compatibility.

Before backend restart, generated direct calls should fail closed when
structured metadata is stale or missing instead of silently reparsing final
spelling.

## In Scope

- Inventory direct-call signature lowering in `src/codegen/lir` and
  `src/backend/bir/lir_to_bir`.
- Select the generated metadata-rich direct-call path and require structured
  signature data there.
- Preserve legacy rendered-signature parsing only for explicit no-metadata
  compatibility fixtures.
- Ensure aggregate return, byval parameter, sret, variadic, and ordinary scalar
  direct-call facts come from structured metadata where available.
- Add focused tests proving structured success and stale/missing metadata
  rejection.

## Out Of Scope

- Rewriting every call ABI path in one slice.
- Changing printer syntax or final emitted call spelling for cosmetic reasons.
- Removing raw direct-LIR compatibility fixtures.
- Reopening parser or sema function-pointer work unless direct-call metadata
  exposes a concrete missing fact.

## Acceptance Criteria

- Metadata-rich direct calls do not depend on parsing rendered function
  signatures for semantic lowering.
- No-metadata compatibility fallback is explicit and fenced.
- Tests cover at least one aggregate-sensitive direct-call case and one stale
  or missing structured-metadata rejection.
- Validation includes focused frontend/LIR/BIR/backend call-signature coverage.

## Reviewer Reject Signals

- The implementation adds a new rendered signature parser branch.
- Generated metadata-rich calls silently fall back to string parsing.
- Tests only assert printed LIR/BIR text.
- The slice expands into broad backend ABI redesign.
