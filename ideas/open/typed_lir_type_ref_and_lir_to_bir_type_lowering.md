# Typed LIR TypeRef and String-Free LIR-to-BIR Type Lowering

Status: Open
Last Updated: 2026-04-03

## Goal

Upgrade the LIR type representation from "text plus coarse classification" to
a structured typed object that retains renderable text while carrying the exact
semantic information needed by downstream lowerings.

As part of that upgrade, change `src/backend/lowering/lir_to_bir.cpp` to use
typed LIR type information as the primary source of truth and remove string
matching as the main lowering decision mechanism for scalar type selection.

## Why This Idea Exists

The current LIR layer already wraps type text in `LirTypeRef`, but the typed
payload is still too weak for backend-facing lowering:

- `LirTypeRef` stores the original text and a coarse `LirTypeKind`
  classification such as `Integer`, `Pointer`, or `Struct`
- that classification does not preserve the exact integer width, pointer
  payload, or other precise shape details needed by `lir_to_bir.cpp`
- `lir_to_bir.cpp` therefore still uses helpers such as
  `lower_scalar_type_text(std::string_view)` to recover `i8`, `i32`, and `i64`
  by string comparison
- this keeps a text-first boundary alive in a place that should be a typed IR
  translation boundary

This is a sign that the front-end/LIR refactor is not yet complete at the type
boundary. The LIR object model exists, but it does not yet carry enough type
information for string-free lowering into BIR.

## Current Problem Surface

Today the backend BIR bridge mixes two worlds:

- function parameters can sometimes lower from preserved `TypeSpec`
- instruction-local types in LIR binary/cmp/cast/ret paths still lower from
  LLVM-style type text such as `"i8"` or `"i32"`

That split causes several problems:

- backend lowering correctness depends on the exact emitted spelling of LIR
  type strings
- type interpretation is duplicated at multiple boundaries instead of being
  decided once when constructing LIR
- `LirTypeRef.kind()` is not rich enough to support BIR lowering directly
- future BIR growth will keep dragging string parsing logic forward unless the
  type boundary is fixed now

## Desired End State

LIR type objects should support both of these needs at once:

- expose a structured typed representation that downstream passes can inspect
  without reparsing text
- preserve a canonical textual rendering for LLVM/LIR printing and debugging

Concretely, after this work:

- integer LIR types should know their exact width, not only "Integer"
- pointer, void, floating, aggregate, function, and opaque/raw cases should
  have explicit typed variants or payloads appropriate to the current LIR model
- instruction forms such as `LirBinOp`, `LirCmpOp`, `LirCastOp`, `LirRet`, and
  function-signature surfaces should expose enough typed type information for
  BIR lowering to operate without string matching as its primary discriminator
- `lir_to_bir.cpp` should lower from typed LIR type data and treat text only as
  a rendering/debugging concern, not as the authoritative semantic source

## Scope

### In Scope

- redesigning `LirTypeRef` so it can carry exact structured type payloads
- deciding how canonical type text is derived from typed data and/or cached
- updating LIR constructors/builders so typed information is attached at object
  creation time rather than reconstructed downstream
- updating LIR verification to validate typed/type-text consistency
- changing `src/backend/lowering/lir_to_bir.cpp` to use typed type information
  instead of `std::string_view` matching for scalar type lowering
- removing now-redundant text-based type decoding helpers from the BIR lowering
  path
- adding regressions that prove BIR lowering behavior no longer depends on type
  string matching

### Out of Scope

- a full semantic redesign of all LIR instructions unrelated to type carriage
- forcing BIR to support more types than its current planned expansion requires
- deleting textual rendering from LIR entirely

## Proposed Approach

### Phase 1: Type model design

- inventory every current `LirTypeRef` use site
- classify which sites only need coarse kind information versus exact type
  payload
- define a typed LIR type model that can represent the currently emitted LLVM
  type strings without losing round-trip renderability

Key design rule:

- type semantics should be determined once, when the LIR type object is formed,
  not re-derived later by ad hoc string checks

### Phase 2: Construct typed types at the source

- update the LIR-producing pipeline so type objects are created with exact typed
  payloads at construction time
- preserve or derive canonical text from the typed model
- ensure existing printer/debug behavior remains readable and stable

### Phase 3: Strengthen verification

- extend `src/codegen/lir/verify.cpp` so it checks the internal consistency of
  the typed payload and any cached/rendered text
- fail early when a type object claims one semantic form but renders as another

### Phase 4: Convert LIR-to-BIR lowering

- replace `lower_scalar_type_text(std::string_view)`-style helpers with typed
  lowering helpers that consume the structured LIR type object
- update `LirBinOp`, `LirCmpOp`, `LirCastOp`, return lowering, and signature
  lowering paths so they use typed type payloads directly
- leave text lookups only where they are unavoidable for diagnostics or legacy
  compatibility, not for primary semantic branching

### Phase 5: Remove text-first fallback logic

- delete or quarantine the old string-based lowering helpers once the typed
  path is complete
- add tests that would fail if type strings were the only source of truth
- audit for other backend paths that still branch primarily on textual type
  spellings and either fix them here or file explicitly separate follow-up ideas

## Acceptance Criteria

- [ ] `LirTypeRef` carries exact typed payloads, not only coarse kind
- [ ] typed LIR type objects can still produce canonical text for printing and
      diagnostics
- [ ] LIR construction attaches type semantics at object creation time
- [ ] `src/codegen/lir/verify.cpp` validates typed/type-text consistency
- [ ] `src/backend/lowering/lir_to_bir.cpp` uses typed LIR type information as
      its primary type discriminator
- [ ] string matching such as `"i8"` / `"i32"` / `"i64"` is no longer the main
      method for scalar type lowering in `lir_to_bir.cpp`
- [ ] focused tests cover typed LIR type creation and typed LIR-to-BIR lowering
- [ ] any remaining unavoidable text-based type handling is explicitly bounded
      and documented as non-primary

## Risks

- this touches a cross-cutting representation boundary, so shallow partial
  conversions could leave the codebase in a worse mixed typed/text state
- if typed payload and text are both stored, drift between them becomes a new
  consistency risk unless verification is upgraded first or in lockstep
- some current backend or printer paths may implicitly rely on exact text
  spellings, which will need careful audit during migration

## Relationship To Existing Work

This is adjacent to, but not identical with, the active BIR expansion idea.

- the active BIR idea focuses on backend IR consolidation and legacy IR removal
- this idea focuses on the earlier LIR type boundary and on removing text-first
  type interpretation from `lir_to_bir.cpp`

The two efforts should align, but this should remain a separate tracked idea so
it does not silently expand the scope of the current active plan.
