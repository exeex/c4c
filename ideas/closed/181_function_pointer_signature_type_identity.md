# Function Pointer Signature Type Identity

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/175_hir_typespec_ref_structured_equivalence.md`
- `ideas/closed/176_lir_type_ref_structured_equality.md`

## Goal

Audit and fix one bounded function-pointer signature identity path so it uses
structured signature facts instead of partial `TypeSpec` shape or rendered
type spelling.

Function pointer types are a high-risk type-identity domain because they mix
return type, parameter type list, variadic state, typedef/record identity, and
call lowering behavior. A raw `is_fn_ptr` flag or rendered signature fragment
is not enough semantic identity once structured type metadata exists.

## Why This Idea Exists

The type identity slices so far covered aggregate layout, aggregate ABI,
ordinary HIR aggregate type-ref equivalence, LIR `LirTypeRef` equality, and
template record owner identity. Function pointer signatures remain a natural
next risk: they appear in parser AST fields, sema canonical signatures, HIR
call inference, LIR calls, BIR indirect calls, and backend ABI lowering.

## In Scope

- Inventory function pointer signature carriers across parser `Node` fields,
  `TypeSpec`, sema canonical types, HIR `FnPtrSig`, LIR calls, and BIR call
  ABI metadata.
- Pick one bounded path, such as indirect call result/argument inference or
  function-pointer parameter passing.
- Carry structured signature identity through that path: return type,
  parameter type refs, variadic/unspecified state, and nominal type identities.
- Preserve rendered signature text as display/diagnostic output.
- Add tests where two rendered-compatible or same-shaped function pointer
  spellings differ by structured parameter/record identity.

## Out Of Scope

- Rewriting all function pointer parsing and lowering.
- Replacing all `TypeSpec` carriers with canonical types.
- Changing final emitted function pointer syntax for cosmetic reasons.
- Weakening indirect-call or ABI tests.

## Acceptance Criteria

- The selected function-pointer path distinguishes syntax payload from
  resolved signature identity.
- Metadata-rich signature comparisons use structured return/parameter facts.
- Missing or mismatched structured signature metadata fails closed or is an
  explicit compatibility boundary.
- Focused tests cover a collision-prone function-pointer signature case.
- Validation includes targeted frontend/HIR/LIR/backend coverage for the
  selected path.

## Closure Summary

Closed after completing the bounded indirect-call LIR/BIR function-pointer
signature path. Metadata-rich indirect calls now carry structured callee
signature facts through LIR call construction and BIR lowering while preserving
rendered call spelling as display/compatibility text. Verification covers
result type, fixed parameter mirrors, call arguments, variadic state,
unspecified parameter lists, stale suffix compatibility, and fail-closed
behavior for mismatched structured metadata.

Remaining parser-only `TypeSpec::is_fn_ptr` comparisons, broad sema canonical
type equality replacement, direct-call signature metadata, final emitted syntax
formatting, and post-BIR backend ABI decisions are outside this idea. Nominal
aggregate function-pointer parameter identity should be treated as follow-up
scope only if frontend lowering later carries aggregate signature refs end to
end.

Closure proof used the focused six-test frontend/LIR/backend guard recorded in
`todo.md`; the close-time regression guard passed with matching 6/6 before and
after results using non-decreasing pass-count mode against the rolled-forward
baseline.

## Reviewer Reject Signals

- Equality still depends on rendered signature text or `is_fn_ptr` alone.
- The route special-cases one named function pointer typedef.
- Tests only assert printed call text.
- The slice becomes a full function-pointer rewrite.
