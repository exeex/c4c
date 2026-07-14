# LIR Memcpy Selected Pointer/Object Authority Publication

Status: Open (active producer blocker for idea 734)
Type: bounded LIR producer/schema/verifier authority repair
Blocked Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one selected, valid current-function `LirMemcpyOp` as a structured
pointer/object operation: destination, source, and size must have typed
identity and each pointer operand must name its owning object and lifetime
contract.  The receiver must be able to consume that single row without
parsing operand display text.

## Why This Exists

Idea 734 is accepted through Step 7.19 (`565be6932`) but its earliest
remaining valid matrix row is `LirMemcpyOp`.  The current carrier has three
`LirOperand` fields whose authority is monostate/text-only.  The existing
verifier checks operand kind but cannot establish current-function use
identity, object ownership, or lifetime.  That is a producer/schema boundary,
not permission for Raw-BIR to recover facts from presentation.

## Selected Row and In Scope

- Select exactly one non-volatile `LirMemcpyOp` emitted by the existing PL
  `emit_lval_dispatch` producer, with a current-function destination object,
  current-function source object, and a native constant i64 size.  Record the
  exact focused fixture/source location in the handoff; do not substitute a
  different producer merely because its display strings look similar.
- Add the minimum typed carrier needed on that operation (or a directly owned
  typed descriptor): destination and source `LirValueId`s, a typed size
  authority (`LirValueId` or `LirIntegerImmediate`), and explicit typed
  destination/source object IDs whose owners are the same `LirFunction`.
- Publish a precise pointer/object/lifetime relation for both operands: each
  operand is a live address of its declared selected object at the memcpy site;
  object IDs are non-invalid, distinct where the selected semantics require
  it, and cannot be borrowed from another function or a dead/out-of-scope
  object.  Compatibility operand text remains display-only.
- Make the reachable LIR verifier reject absent, invalid, duplicate or
  cross-function value/object IDs, type/kind disagreement, incoherent
  value-to-object association, non-i64/nonpositive selected size authority,
  and lifetime/owner violations before any consumer may rely on the row.
- Add focused producer/verifier coverage for the selected valid row and nearby
  malformed authority cases, including transactional failure with no partial
  selected-row publication.
- Publish a durable handoff for idea 734 naming the exact fields, selected
  producer, object/lifetime ownership rules, accepted focused proof, and
  fail-closed boundary.

## Out Of Scope

- Raw-BIR containers, importer dispatch, Raw-BIR verification, receiver tests,
  target lowering, canonical BIR, allocation, MIR, or emission.
- Any second `LirMemcpyOp`, volatile memcpy, dynamic-size form, alias analysis,
  overlap semantics, generic pointer/object model, stack/alloca family,
  globals, parameters, va-list, memset, loads/stores/GEPs, aggregate/vector,
  CFG, or opaque inline-assembly authority.
- Recovering a value, object, size, lifetime, or ownership relation from
  `LirOperand::str()`, rendered LLVM, test names, or a parallel text table.

## Acceptance Criteria

- One selected PL memcpy row carries typed current-function destination,
  source, and size authority plus explicit typed object IDs and live ownership
  relations for both pointers.
- The LIR verifier makes that row fail closed for missing, invalid,
  cross-function, mismatched, and lifetime-invalid authority, with no
  presentation fallback.
- Focused producer/verifier proof covers the valid row and neighboring failure
  boundary, without weakening existing contracts or naming a testcase as the
  implementation rule.
- The handoff lets idea 734 resume at repaired Step 7.20 to receive only this
  published row.  It does not claim Raw-BIR receipt or general memory support.

## Handoff To Idea 734

After acceptance, reactivate
`ideas/open/734_lir_to_new_bir_container_completeness.md` at `Step 7.20`.
That step may receive only this handoff's selected `LirMemcpyOp` into a typed
Raw-BIR container, importer dispatch, reachable Raw-BIR verifier ownership,
and positive/negative transactional proof.  All other memory/object and
adjacent families remain separate and fail closed.

## Reviewer Reject Signals

- Reject text parsing, rendered-IR comparison, name lookup, or testcase-shaped
  matching offered as operand, object, size, or lifetime authority.
- Reject a carrier that has value IDs but leaves object owner/lifetime
  unverifiable, or an object table that is not bound to the selected operands.
- Reject widening to a generic pointer/object framework, a second memcpy row,
  stack/va-list/memset/load/store/GEP work, or any Raw-BIR receiver change.
- Reject expectation downgrades, helper-only refactors, or tests that omit
  missing/invalid/cross-owner/lifetime rejection while claiming capability.
- Reject publication that permits partial selected-row state after verifier
  failure or that retains the old monostate/text path as semantic fallback.
