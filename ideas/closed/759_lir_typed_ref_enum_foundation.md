# LIR typed ref enum foundation

## Intent

Introduce typed enum carriers for LIR reference wrappers so core LIR metadata
can be constructed and queried through stable ids instead of relying on string
classification as the primary authority.

This first slice is a foundation change. It should add the enum model and
constructors, but it should not attempt to migrate every call site away from
string input.

## Why This Exists

`LirTypeRef` and related LIR ref classes currently keep rendered text as the
main source of truth, then parse or compare strings later to recover semantic
identity. That made the recent LIR emission regression harder to reason about:
type authority and textual LLVM spelling were coupled too tightly.

A prior compile experiment showed that removing runtime string construction at
once is too broad. Runtime type text still arrives from HIR `TypeSpec` lowering,
struct layout rendering such as `[N x i8]`, parsed call argument views, extern
signature text, and verification fallback paths. The first step should
therefore make enum construction available and authoritative where possible
without breaking those remaining runtime text boundaries.

## In Scope

- Update `src/codegen/lir/types.hpp`.
- Add enum carriers for `LirTypeRef` builtins such as void, pointer, integer
  widths, and floating scalar types.
- Add enum constructors for `LirTypeRef` and keep existing string construction
  available for runtime text that cannot yet be represented as an enum.
- Preserve rendered text output as an emission boundary, but make enum/id state
  available as the preferred typed query path after construction.
- Extend the same enum-constructor pattern to nearby related ref classes in
  `types.hpp`, including opcode/predicate-style wrappers such as
  `LirBinaryOpcodeRef`, when they already have a closed semantic set.
- Keep the change minimal enough that existing LIR generation, verifier, and
  backend tests continue to compile without broad call-site migration.

## Out Of Scope

- Removing runtime string constructors from `LirTypeRef`.
- Migrating every existing string call site to enum construction.
- Replacing HIR `TypeSpec` to LLVM type rendering.
- Reworking aggregate, vector, struct, or function type representation beyond
  preserving their current runtime text path.
- Changing verifier rules, BIR semantics, LLVM IR emission semantics, or test
  expectations unrelated to typed ref construction.

## Acceptance Criteria

- `LirTypeRef` has an enum/id carrier for closed builtin scalar/pointer/void
  types and exposes a typed query path that does not require reparsing text.
- `LirTypeRef` can be constructed directly from the new enum/id values.
- Existing string and runtime text construction still works for currently
  supported dynamic type spellings.
- Related closed-set wrappers in `src/codegen/lir/types.hpp`, such as binary
  opcode and comparison predicate refs, have equivalent enum-first
  construction where practical.
- The repository builds with the normal configured compiler mode.
- Focused LIR/frontend/backend tests that exercise `LirTypeRef` construction,
  printing, verification, and lowering continue to pass.

## Follow-Up

After this idea is accepted and closed, the next intended source idea is
`ideas/open/760_lir_string_constructor_deprecation_migration.md`. That follow-up
uses the enum/ref foundation from this idea to add targeted `[[deprecated]]`
coverage and migrate string construction call sites in bounded groups.

## Closure Record

Close accepted: the enum-first foundation is complete. The accepted route was
audited in `f86b3cfb7`, implemented in `bf75f2603`, and given focused authority
proof in `d411270f4`; final validation was recorded in `b2ffae838`. A fresh
build and six focused LIR frontend/backend tests passed, and the matching
`frontend_lir_call_type_ref` before/after regression guards passed earlier in
the route. The accepted work retains runtime string construction, avoids broad
caller migration, and makes no verifier, expectation, or unrelated semantic
change.

`ideas/open/760_lir_string_constructor_deprecation_migration.md` remains open
as the successor only for its distinct targeted deprecation and call-site
migration scope. It is not remaining work required to complete this foundation.

## Evidence Notes For Later Execution

The runtime string call-site experiment identified these important future
migration boundaries:

- HIR `TypeSpec` lowering produces strings through helpers like `llvm_ty(ts)`.
- struct layout code generates dynamic spellings such as `[N x i8]`.
- parsed typed call argument views re-own `arg.type` as text.
- extern declarations keep return type text and later reconstruct
  `LirTypeRef`.
- verifier and BIR lowering still classify `type.str()` as a compatibility
  fallback.

These should remain supported in this foundation idea and become targets for a
later migration idea.

## Reviewer Reject Signals

- The change removes runtime string construction or requires broad call-site
  migration in this foundation slice.
- The enum/id value is only decorative while verifier or type queries still
  reparse text for closed builtin types.
- The patch weakens verifier checks, downgrades tests, or changes expected
  supported behavior to make the enum addition pass.
- The implementation introduces testcase-shaped special cases for only one
  failing LIR path instead of a general typed ref construction model.
- The patch rewrites unrelated HIR, BIR, backend, or LLVM emission behavior.
- String rendering is removed from the emission boundary before dynamic
  aggregate/vector/struct/function types have an explicit typed replacement.
