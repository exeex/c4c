# Local Operation Authority Handoff To Idea 734

This completed Steps 1–3 handoff from idea 792 authorizes exactly one new,
receiver-ready producer-backed LIR row: the VLA `LirStackSaveOp`
saved-stack-pointer result. It neither authorizes nor combines the later
`LirStackRestoreOp` use. The accepted alloca, local-load, declaration-store,
and static-local-array-GEP receipts remain historical work only.

## Historical 791 receipt: static-local-array GEP

The prior direct static-local-array `LirGepOp` receipt, emitted for an
integer-immediate index, is retained here as history. Its receiver inputs were
native fields, never operand text:

| Field | Required fact |
| --- | --- |
| `requires_native_local_gep_authority` | True for this selected producer route. |
| `requires_native_result_authority` / `result` | True plus a valid current-function `LirValueId` result. |
| `element_type` | Exact `LirTypeRef` of the indexed array element, equal to `local_object_authority.indexed_element_type`. |
| `ptr` | Native SSA `LirValueId` equal to `local_object_authority.pointer_definition`. |
| `indices` | Exactly one authoritative `i64` native `LirIntegerImmediate` index. |
| `local_object_authority` | Valid/live current-function `object`, `owner`, pointer definition/type, and pointee type. |

The verifier checks selected-GEP admission, native result/base/index shape,
matching published array-element type, and the existing current-function
pointer/object/owner/liveness binding. Presentation is nonsemantic: a receiver
must not recover or validate facts through a local name, `%t`, rendered operand,
printer output, LLVM text, or testcase identity.

### Historical guarantees and rejected forms

The receiver may consume only the documented result, element type, base pointer,
immediate index, and checked local-object authority transactionally. Missing
admission/local authority, raw or SSA index, invalid result/base, foreign or
incoherent object/owner, type mismatch, and dead authority reject before
downstream use.

All stores, SSA-indexed/local-temporary/aggregate-member GEPs, VLA GEPs, every
later load, stack-save/restore, and all other local-operation families remain
unsupported and fail closed.

### Historical evidence

- Producer/schema/verifier selection: current idea 791 Steps 1–3 worktree.
- Focused proof: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`.

### Historical Step 7.29 return action

Resume 734 at **Step 7.29 - Receive the selected direct static-local-array
`LirGepOp` authority**. Add only the minimum typed Raw-BIR destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage for these native fields. Do not repeat the accepted alloca, local-load,
or declaration-store receipts and do not use presentation recovery.

## Completed 792 receipt: VLA stack-save saved-stack-pointer authority

The sole 792 receiver row is the VLA `LirStackSaveOp` that `init_fn_ctx` emits
before the function body when the function has VLA locals. It is exactly one
row: its saved-stack-pointer result. No other save result, use, or local
operation is admitted by this handoff.

| Native field | Established authority |
| --- | --- |
| `result` | A valid current-function SSA `LirValueId`, published as the `LirStackSaveOp` result. |
| `local_object_authority.pointer_definition` | Exactly the result value identity, so the verifier binds the receipt to its producer. |
| `local_object_authority.object` / `owner` | A newly allocated local object owned by the current `LirFunction`; the verifier requires a unique matching function owner. |
| `local_object_authority.pointer_type` / `pointee_type` | Native `ptr` / `ptr` type facts; `LirStackSaveOp` is modeled as a current-function pointer result. |
| `local_object_authority.live` | True at publication and required true by the local-authority verifier. |
| result/use and liveness verification | `verify_local_object_authority_bindings` verifies the result-to-pointer-definition binding and current-function pointer definition; `verify_function_value_ownership` reaches that binding verifier. |

`init_fn_ctx` creates the authority without parsing a local name or rendered
operand. Step 2 requires this authority for the selected row, so absent
authority rejects before receiver use. The verifier rejects invalid result
identity, foreign or non-unique owner, non-pointer authority, dead authority,
and a pointer definition that is not the save result. The receiver guarantee is
therefore one typed current-function saved-stack-pointer definition with a
native object/owner/pointer-type/pointee-type/liveness receipt; it may consume
only those documented native fields transactionally and never presentation.

Rejected and still fail-closed: the historical static-local-array GEP; all
ordinary local load/store and nonselected GEP forms (SSA-indexed,
local-temporary, aggregate-member, and VLA GEP); VLA dynamic alloca and its
pointer store; every `LirStackRestoreOp`; and any second or otherwise
nonselected `LirStackSaveOp`. Stack restore has a native saved-pointer use and
authority binding, but is deliberately not selected: it is the separate
lifetime-consumer row, not part of this save-result receipt.

## Focused producer proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`

This is the bounded producer-slice proof for the selected VLA stack-save row;
the proof log is `test_after.log`.

## Exact Step 7.30 return action for the 792 selection

Resume 734 at **Step 7.30 - Receive the selected VLA `LirStackSaveOp`
authority**. Add only the minimum typed Raw-BIR saved-stack-pointer destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage for the native result and local object/owner/pointer-type/pointee-type/
liveness fields above. Do not receive stack restore, dynamic VLA allocation, or
any other local operation, and do not recover authority from presentation.

## 798 return handoff to 794: selected VLA stack restore only

This is a return handoff to **794**, not a receipt for 734.  Commit
`cdeacb2cd` publishes exactly one selected lifetime-consumer row:
`LirStackRestoreOp` emitted for the backward-VLA-goto route.  Its selected
admission is `requires_native_stack_restore_authority == true`; no name,
rendered operand, LLVM text, or testcase identity selects the row.

| Native field | Required selected fact |
| --- | --- |
| `saved_ptr` / `local_object_authority.pointer_definition` | A valid SSA saved-pointer ID, equal to the authority pointer definition and a modeled current-function pointer definition. |
| `local_object_authority.object` / `owner` | A valid current-function object with the unique matching current-function owner. |
| `local_object_authority.pointer_type` / `pointee_type` / `live` | Native `ptr` / `ptr` type facts and a live checkpoint binding. |
| `lifetime_transition` | Present only for the selected row, with kind `RestoreSavedVlaStackCheckpoint` and `saved_pointer_definition` equal to both `saved_ptr` and the authority pointer definition. |

The operation-local `RestoreSavedVlaStackCheckpoint` transition records the
consumption of this saved VLA stack checkpoint.  It does not create a
per-dynamic-VLA allocation-lifetime model or change `live` into a
post-transition state.

The verifier rejects malformed or unselected-field combinations, non-SSA,
invalid, or unbound saved pointers, invalid object, foreign/non-unique owner,
non-pointer pointer or pointee types, non-live authority, and invalid or
mismatched transition kind/definition forms.  The focused proof was
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`;
it passed with the focused guard non-decreasing at **1/1**.

### Exact return action

794 may now return at **Step 2 only** to publish and verify this selected
native stack-restore authority, or complete its own handoff process.  This
does not authorize Raw-BIR/importer/734 receipt, dynamic-VLA count work, VLA
GEP, or any other local/VLA row.

## 794 handoff to 734: selected VLA stack restore only

This is the one receiver-ready handoff from **794** to **734**. It authorizes
exactly the selected `LirStackRestoreOp` emitted for the backward-VLA-goto
route; it is not a Raw-BIR/importer implementation or a 734 receipt. Selection
is native and explicit: `requires_native_stack_restore_authority == true`.
No local name, rendered operand, LLVM text, testcase identity, `monostate`, or
unresolved classification may select or supplement this row.

| Native field | Receiver guarantee |
| --- | --- |
| `requires_native_stack_restore_authority` | Must be true; this is the sole selected-row admission. |
| `saved_ptr` | A valid current-function SSA saved-pointer `LirValueId`. |
| `local_object_authority.pointer_definition` | Equal to `saved_ptr`, and a modeled current-function pointer definition. |
| `local_object_authority.object` / `owner` | A valid current-function object and its unique matching current-function owner. |
| `local_object_authority.pointer_type` / `pointee_type` / `live` | Native `ptr` / `ptr` facts and a live saved-checkpoint binding. `live` is binding validity, not a per-dynamic-VLA allocation lifetime state. |
| `lifetime_transition` | Present for this selected row only; kind is `RestoreSavedVlaStackCheckpoint`, and `saved_pointer_definition` equals both `saved_ptr` and `local_object_authority.pointer_definition`. |

734 may consume these fields transactionally for one typed stack-restore
destination, importer dispatch, reachable verification, and nearby
positive/negative receiver coverage. The producer verifier rejects absent or
unselected admission; non-SSA, invalid, unbound, or mismatched saved pointers;
invalid object; foreign or non-unique owner; non-pointer pointer/pointee types;
non-live authority; and missing, invalid-kind, or definition-mismatched
transitions. Each such form remains fail closed before downstream use.

Everything outside that one row remains excluded: dynamic-VLA count/allocation,
VLA GEP, any other stack save or restore, local load/store/GEP, local
temporaries, other lifetime consumers, Raw-BIR work beyond the one future 734
packet, target lowering, MIR, and presentation-derived recovery.

### Accepted producer proof and exact receiver return

The producer contract, implementation, and original return handoff are
`8bd881842`, `cdeacb2cd`, and `f5cfa52b7`. 794 preserved that selected-only
contract through its completed Step 2 (`b2e2e5594`), with fresh focused proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`

The command passed 1/1. Resume 734 only at **Step 7.31 - Receive the selected
VLA `LirStackRestoreOp` authority**. Do not treat this handoff as evidence that
734 has received, verified, lowered, or otherwise implemented the row.
