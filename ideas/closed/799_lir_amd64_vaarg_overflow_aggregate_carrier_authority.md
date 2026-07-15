# AMD64 `va_arg` Overflow Aggregate Carrier Authority

Status: Closed
Type: bounded derived-pointer/object/lifetime authority blocker
Parent: `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`

## Goal

Publish one checked, structured AMD64 SysV overflow-area aggregate `va_arg`
carrier so its required memcpy-like move has native derived-pointer, object,
lifetime, and typed-size authority.

## Why This Exists

753's required memcpy-like `va_arg` evidence reaches
`emit_amd64_va_arg_from_overflow` in
`src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`. Its
`LirMemcpyOp{tmp_addr, stack_ptr, size}` uses `stack_ptr`, a pointer derived
from the overflow-area cursor rather than a current-function local-object
pointer. 753 expressly excludes aggregate/vector carrier publication, so this
route must be established here before 753 can publish that one matching row.

## In Scope

- Define and publish the minimum structured authority for the derived AMD64
  overflow-area source pointer used by the aggregate `va_arg` memcpy-like row:
  derivation/base relation, current-function ownership, source object or
  storage identity, lifetime, and exact typed byte size.
- Verify malformed, foreign, dead, non-overflow-derived, type-incoherent, and
  size-incoherent carrier facts before `LirMemcpyOp` construction; unsupported
  aggregate forms must fail closed or remain compatibility-only.
- Add nearby positive and negative AMD64 `va_arg` aggregate coverage proving
  that the accepted overflow route emits the checked memcpy-like move without
  presentation recovery.
- Hand 753 an explicit checked-carrier contract for exactly this aggregate
  overflow memcpy row, including facts, rejects, and proof references.

## Out Of Scope

- General aggregate/vector result or operand identity, other ABI targets,
  scalar/pointer `va_arg`, `va_start`, `va_end`, `va_copy`, generic memcpy or
  memset authority, Raw-BIR receiver work, MIR, emission, or full ABI/memory
  model redesign.
- Changing 753's source scope or claiming its acceptance; 753 alone resumes
  and selects the matching producer packet after this blocker is accepted.
- Deriving pointer, object, lifetime, size, or row-selection facts from
  builtin names, operand spelling, rendered LIR/LLVM, testcase names, or
  compatibility text.

## Acceptance Criteria

- The selected AMD64 aggregate `va_arg` overflow path constructs its memcpy-like
  carrier from checked structured derived-pointer/object/lifetime and typed-size
  facts, not from a local-object-only assumption or presentation recovery.
- The verifier rejects malformed carrier provenance, foreign/dead ownership,
  and type/size mismatch before downstream use; unsupported aggregate routes
  remain fail closed or explicitly compatibility-only.
- Focused positive and negative coverage proves the accepted aggregate overflow
  row and its rejection boundary. A fresh build and the selected focused proof
  pass before handoff.
- The handoff gives 753 the exact native fields and guarantees needed to resume
  Step 2 for this one row; it does not silently publish aggregate/vector work
  beyond the carrier boundary.

## Reviewer Reject Signals

- Reject a patch that treats `stack_ptr` as a current-function local-object
  pointer without a checked overflow derivation/base, object, and lifetime
  relation.
- Reject reconstructed facts from printed operands, builtin names, rendered
  LIR/LLVM, named testcases, or a text-shaped aggregate special case.
- Reject accepting a type or byte-size mismatch, foreign/dead source, or an
  arbitrary derived pointer merely to make the selected move lower.
- Reject a broad aggregate/vector authority rewrite, target-general ABI change,
  or a change to 753's scope claimed as progress for this blocker.
- Reject coverage that proves only a named fixture while malformed or
  unsupported carrier authority remains accepted.

## Closure Record

Disposition: capability complete; closed after Step 3 handoff.

- **Accepted implementation:** `c4e820a48` publishes one optional
  `LirAmd64SysVOverflowAggregateCarrier` only for a checked direct-local AMD64
  SysV aggregate `layout.needs_memory` overflow memcpy. Its native facts are
  the `va_list` local authority, typed field-2 GEP, overflow pointer load,
  overflow-area storage kind, destination local temporary, final load, struct
  payload type, and positive typed i64 payload size.
- **Guarantees and rejects:** the carrier is exclusive to a selected,
  non-volatile memcpy and the verifier requires the canonical local facts and
  direct field-2/load/destination/load chain. It rejects partial or unselected
  carrier fields, arbitrary/non-derived sources, foreign/dead locals,
  destination disagreement, and payload type/size mismatch. Nonmatching
  aggregate/vector and target routes remain compatibility-only.
- **Accepted proof:** `cmake --build --preset default &&
  ./build/tests/backend/bir/backend_lir_selected_pointer_authority_test`
  passed; `test_after.log` records the matching 5/5 backend baseline.
- **Parent handoff:** 753 resumes at Step 2 and may consume only this matching
  producer carrier; 799 does not authorize republishing or generalizing
  aggregate/vector authority. The exact return point and remaining work are
  recorded in 753's *Resumed Carrier Handoff Record*.
