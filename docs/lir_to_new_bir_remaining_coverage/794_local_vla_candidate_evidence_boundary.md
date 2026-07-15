# 794 Local/VLA Candidate Evidence Boundary

Status: Step 1 evidence gate complete; no candidate selected.

## Decision

No remaining local/VLA candidate has an admissible, uniquely bounded native
current-function authority contract.  This record selects no Raw-BIR row and
does not authorize a 734 return.  The checked precondition is stricter than a
typed display operand or an optional local-object record: the candidate needs
one explicit row identity plus native value/object/owner/type/liveness facts
that describe the complete operation, including its consumer or dynamic
operand facts where applicable.

Closed 792 remains historical authority for exactly one `LirStackSaveOp`
saved-stack-pointer result.  Its `LirStackRestoreOp`, dynamic allocation, VLA
GEP, pointer-slot store, and every other local row are expressly outside that
receipt.  The successor queue and first-owner matrix likewise name no
receiver-ready local/VLA row.

## Native Evidence Checked

`LirCurrentFunctionLocalObjectPointer` supplies a valid current-function
pointer definition, local object, unique function owner, pointer and pointee
types, and `live`; `verify_local_object_authority` and
`verify_local_object_authority_bindings` check these facts against a modeled
pointer definition.  That common object record is not a row selector and does
not supply operation-specific value/use or lifetime facts.

| Candidate class | Current native structured facts | Boundary result / first missing fact |
| --- | --- | --- |
| Stack restore (`LirStackRestoreOp`) | `saved_ptr` is an SSA `LirOperand`; its local authority binds that ID to a current-function pointer definition, object, owner, pointer/pointee type, and `live`.  `StmtEmitter` emits it from the saved-stack authority on a backward VLA goto. | Fail closed.  There is no `requires_native_stack_restore_authority` (or equivalent selected-row discriminator) and no structured lifetime-consumer/transition fact saying what lifetime ends after this use.  The existing authority proves the saved pointer, not one receiver-safe restore event. |
| Dynamic VLA allocation (`LirAllocaOp`) | The dynamic result has an SSA `LirValueId`; its local authority is updated with a current-function object, owner, pointer type, element pointee type, and `live`. | Fail closed.  `StmtEmitter` passes the computed `count` as a string into `LirAllocaOp`; it is not published as a native current-function value/type/definition contract.  The first missing owner fact is the dynamic-count value identity and typed definition (including any scale/coercion), bound to this alloca. |
| VLA GEP | The VLA declaration has a local pointer-slot authority, and non-VLA local array GEPs can use the separately selected immediate-index path. | Fail closed.  VLA bases deliberately do not enter the structured local-array base route; the remaining GEP route does not publish a VLA pointer/object binding, native result identity, and authoritative index value/type together.  The first missing owner fact is a VLA-GEP base/result/index authority tuple. |
| Nonselected local load | A direct local pointer can carry local object/owner/type/liveness facts; some loads can carry a `fresh_value` result ID. | Fail closed.  The generic load record has no selected local-load discriminator, and compatibility loads remain outside `requires_native_result_authority`; there is no one-row admission that binds result, source pointer/object, load type, and liveness for a remaining load variant. |
| Nonselected local store (including VLA pointer-slot store) | Direct local stores may bind their pointer to local object/owner/type/liveness facts.  The VLA pointer-slot store has a dynamic-pointer SSA value and a slot authority. | Fail closed.  Only the historical selected integer-immediate local-store shape has `requires_native_store_authority`; other store values have no complete native value-use/type contract.  The first missing owner fact is the stored-value identity/type (and, for the VLA slot, its ownership/lifetime relation) under an explicit selected-store admission. |
| Nonselected local GEP | The historical immediate local-array GEP can carry result/base/immediate-index and local authority under `requires_native_local_gep_authority`. | Fail closed.  That admission is the closed historical row, not a general local/VLA GEP contract.  SSA-indexed, local-temporary, aggregate-member, and VLA forms lack the selected row's complete native base/result/index tuple. |
| Local temporaries | `fresh_tmp` produces presentation spelling; selected value-producing routes use `fresh_value` separately.  Many temporary-producing local operations are still constructed from strings. | Fail closed.  A temporary spelling is not a `LirValueId`; the first missing owner fact is a current-function value definition and type for the temporary, with an operation-specific object/use binding. |
| Other lifetime consumers | The verifier recognizes `LirStackRestoreOp.saved_ptr` as a modeled value use and can validate an attached local-object authority. | Fail closed.  It has no generic structured lifetime-event kind, pre/post liveness state, or selected-consumer admission, so it cannot establish a single remaining consumer row. |

## Route Needed

The earliest blocker is producer/schema/verifier publication of one explicit
operation-specific local/VLA admission.  If the next route is stack restore,
it must add a selected restore identity and native lifetime-consumer transition
facts in addition to the existing saved-pointer/object/owner/type binding.  If
the next route is dynamic allocation, it must first publish the typed dynamic
count definition.  A plan-owner must choose and scope that separate blocker;
794 Step 2 cannot begin from the current evidence because doing so would choose
among several fail-closed variants without a complete native contract.

## Evidence Locations

- `src/codegen/lir/ir.hpp`: operation fields and local-object authority
  carriers.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`: VLA pointer-slot and stack-save
  publication.
- `src/codegen/lir/hir_to_lir/stmt.cpp`: dynamic VLA allocation, pointer-slot
  store, and stack-restore emission.
- `src/codegen/lir/hir_to_lir/lvalue.cpp`: selected local-array GEP boundary
  and generic local load/GEP construction.
- `src/codegen/lir/verify.cpp`: local authority binding and the existing
  stack-save/local-store/local-GEP admission checks.
- `docs/lir_local_operation_authority/handoff_to_734.md`, closed 792,
  `successor_queue.md`, and `first_owner_matrix.md`: historical selection and
  successor boundary.
