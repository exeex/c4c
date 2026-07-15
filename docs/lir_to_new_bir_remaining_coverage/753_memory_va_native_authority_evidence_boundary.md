# 753 Native Memory/VA Authority Evidence Boundary

Status: Step 1 evidence gate complete.  This is a producer/schema/verifier
handoff for Step 2 only; it publishes no Raw-BIR receipt and selects no future
receiver row.

## Boundary

Closed 752 supplies `LirCurrentFunctionLocalObjectPointer`: a current-function
pointer definition, local object, unique owner, pointer and pointee types, and
live fact.  Its verifier path checks the record and, where it is attached,
binds the record's pointer definition to a modeled current-function pointer
operand.  Those facts are usable native substrate, but are not a generic
memory/VA row selector or resolver.

The only existing memory-specific structured operation authority is the
historical selected fixed-aggregate-byval `LirMemcpyOp` descriptor.  It binds
two pointer IDs, two objects/owners/live facts, and a positive i64 immediate;
the function-level selected-memcpy verifier checks exactly one such
non-volatile row against `selected_memcpy_pointer_authority`.  That accepted
row remains historical evidence only.  It neither authorizes another memcpy
row nor supplies authority for memset or VA operations.

All facts below are native structured fields only.  Builtin names, operand
spellings, rendered LIR/LLVM, testcase identity, `monostate`, and unresolved
classification are excluded from selection, publication, and verification.

## Named Family Inventory

| Family | Present native facts | First missing authority fact / Step 2 boundary |
| --- | --- | --- |
| `memcpy` | The historical selected row has `selected_authority` with destination/source `LirValueId`, object IDs, owners, live facts, and i64 positive size.  Every other `LirMemcpyOp` has only display-compatible `dst`, `src`, `size`, and volatility fields. | An operation-local opt-in admission plus a descriptor that binds this row's destination and source operands to current-function pointer/object/owner/type/live facts and binds its size to a native i64 value/immediate fact.  Unselected memcpy remains compatibility-only; Step 2 must not reuse the historical selected descriptor as a general row selector. |
| `memset` | `LirMemsetOp` has destination, byte-value, size, and volatility carriers; aggregate-zero producers currently pass compatibility operands. | A selected-operation admission and a destination pointer/object/owner/type/live binding, plus native typed byte-value and i64 size facts bound to this operation. |
| `va_start` | `LirVaStartOp` has one `ap_ptr` carrier. | A selected-operation admission and a current-function va-list pointer/object/owner/type/live binding for `ap_ptr`.  No va-list state transition is presently structured, so Step 2 may validate the pointer authority only and must not invent lifecycle semantics. |
| `va_end` | `LirVaEndOp` has one `ap_ptr` carrier. | A selected-operation admission and a current-function va-list pointer/object/owner/type/live binding for `ap_ptr`.  The current substrate has no native va-list ended-state fact; remain fail closed rather than infer it. |
| `va_copy` | `LirVaCopyOp` has destination and source pointer carriers. | A selected-operation admission and two native current-function pointer/object/owner/type/live bindings, each tied to its respective operand and distinguished by destination/source role. |
| `va_arg` | `LirVaArgOp` has `result`, `ap_ptr`, and `LirTypeRef type_str`.  The semantic scalar/pointer producer can give `result` a native value ID and type, but its `ap_ptr` is not bound to local-object authority; other VA-arg constructions remain compatibility forms. | A selected-operation admission and an `ap_ptr` pointer/object/owner/type/live binding, plus a native result ID/type relation when the selected producer emits a result.  The aggregate VA paths that emit memcpy-like moves have no such complete tuple and remain compatibility-only. |

## Verifier Boundary

`verify_inst` currently checks these families only as pointer/value/result
operand kinds (and the VA-arg type reference).  The only operation-specific
function-level authority pass is the historical selected-memcpy verifier.
Consequently, the first missing malformed-authority checks for every residual
family are operation-local admission consistency, invalid/missing pointer or
object, foreign/non-unique owner, non-pointer or pointee/type disagreement,
dead authority, operand-to-authority mismatch, and typed value/size mismatch
where the operation has a byte, size, or result fact.  Selected descriptors
must fail before downstream use; an unselected operation carrying selected
fields must also reject so compatibility cannot become an implicit fallback.

## Minimal Step 2 Producer Packet

Add one bounded, native-only authority descriptor pattern to the six named LIR
operation schemas, guarded by per-operation opt-in admission.  Populate it
only from a producer that can retain a current-function pointer operand and
derive the exact 752 local pointer/object/owner/type/live record without
presentation recovery.  Preserve existing operand text as a rendering mirror.
Add reachable LIR verification that checks the descriptor and its binding to
the operation before use, with positive and malformed-authority coverage for
each admitted form.

This packet does not choose a Raw-BIR receiver, does not publish or verify a
receiver handoff, and does not select multiple future rows.  Any named
producer that cannot supply the complete native tuple above remains unselected
and fail closed or explicitly compatibility-only.  In particular, the
historical selected memcpy descriptor is not widened.

## Evidence Locations

- `src/codegen/lir/ir.hpp`: local-object substrate, the historical selected
  memcpy descriptor, and all six operation schemas.
- `src/codegen/lir/hir_to_lir/lvalue.cpp` and `stmt.cpp`: memset and historical
  selected memcpy emission.
- `src/codegen/lir/hir_to_lir/call/builtin.cpp`: native producer seams for
  memcpy, va-start, va-end, and va-copy.
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp`: semantic and compatibility
  VA-arg seams, including aggregate memcpy-like moves.
- `src/codegen/lir/verify.cpp`: baseline operand-kind verification, 752 local
  authority binding, and the historical selected-memcpy verifier.
