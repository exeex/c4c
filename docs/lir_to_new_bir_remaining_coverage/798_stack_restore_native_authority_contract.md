# 798 Native Stack-Restore Authority Contract

Status: Step 1 producer/schema contract for the selected `LirStackRestoreOp`
row.  This note is an implementation handoff for plan Step 2 only; it does
not publish a Raw-BIR receipt or select another local/VLA row.

## Evidence Boundary

The current emitter creates a `LirStackRestoreOp` only while emitting a
`GotoStmt` when the function has a VLA stack-save pointer and the resolved
target is backward or the current block (`stmt.cpp`,
`StmtEmitter::emit_control_flow_stmt`).  It supplies an SSA `saved_ptr` whose
`LirValueId` is the `pointer_definition` of
`FnCtx::vla_stack_lifetime_authority`, and copies that authority into the
operation.  `init_fn_ctx` created that authority with the selected VLA
`LirStackSaveOp`: a current-function object and owner, `ptr` pointer type,
`ptr` pointee type, and `live == true`.

The current verifier already does two relevant checks:

- `verify_local_object_authority` requires a valid pointer definition and
  object, one current-function owner, pointer type, and `live` authority.
- `verify_local_object_authority_bindings` makes the restore's `saved_ptr`
  equal that pointer definition and requires its defining instruction to be a
  modeled current-function pointer result.

These checks bind a saved pointer; they do **not** select a restore row.
`LirStackRestoreOp` currently has only `saved_ptr` and optional
`local_object_authority`, and `verify_inst` currently checks only that
`saved_ptr` is a pointer operand.  There is no lifetime-event kind, no
transition carrier, and no restore-specific admission.

## Contract

### Current fields reused unchanged

The selected form reuses these current native facts without parsing operand
text, a name, LLVM text, or testcase identity:

| Current field | Required selected fact |
| --- | --- |
| `LirStackRestoreOp.saved_ptr` | Valid SSA operand whose `LirValueId` is the saved checkpoint definition. |
| `local_object_authority.pointer_definition` | Exactly `saved_ptr`'s ID and a modeled current-function pointer definition. |
| `local_object_authority.object` / `owner` | Valid object and the unique current function's `link_name_id`. |
| `local_object_authority.pointer_type` / `pointee_type` | Both native pointer (`ptr`) type facts, matching the selected VLA stack-save authority. |
| `local_object_authority.live` | `true` at the restore binding.  This remains a validity fact for the checkpoint binding. |

### Proposed minimal schema fields

Add only the following restore-specific fields to `LirStackRestoreOp`:

```cpp
bool requires_native_stack_restore_authority = false;
std::optional<LirStackRestoreLifetimeTransition> lifetime_transition;

struct LirStackRestoreLifetimeTransition {
  enum class Kind { RestoreSavedVlaStackCheckpoint } kind;
  LirValueId saved_pointer_definition = LirValueId::invalid();
};
```

`requires_native_stack_restore_authority` is the sole selected-operation
admission.  `lifetime_transition` is mandatory exactly when that admission is
true.  Its only admitted kind says that this operation consumes the saved VLA
stack checkpoint by restoring to it; its `saved_pointer_definition` must equal
both `saved_ptr.value_id()` and
`local_object_authority.pointer_definition`.

This is deliberately an operation-local transition, not a generic lifetime
model.  Current LIR has no native identity or mutable liveness state for every
dynamic VLA allocation that the restore could enumerate.  Therefore the
transition does **not** assert that a particular alloca object, or all VLA
objects, changes from live to dead.  It records only the native, selected
consumer effect justified by the emitter: restoring to the already-bound saved
VLA stack checkpoint.  In particular, `local_object_authority.live` is not a
post-transition state and must not be flipped by this packet.

### Producer and verifier obligations

For the existing backward-VLA-goto emission path, the producer must set the
admission true, preserve the current authority copied from
`vla_stack_lifetime_authority`, and emit the one transition above using that
authority's pointer-definition ID.  All other existing restore construction
remains compatibility/unselected only when it carries none of the proposed
restore-authority fields.

The Step 2 verifier packet must add a restore-specific helper alongside the
existing native stack-save helper and invoke it from `verify_inst`; it must
also retain the existing function-level local-authority and binding passes.
The helper must reject:

- a selected restore without local authority or without a transition, and an
  unselected restore that carries either restore-specific authority field;
- a non-SSA, invalid, missing, or mismatched `saved_ptr` ID;
- an invalid/foreign/non-unique owner; invalid object; non-pointer pointer or
  pointee type; non-live authority; or a saved pointer that is not a modeled
  current-function pointer definition;
- a transition whose kind is not `RestoreSavedVlaStackCheckpoint`, whose ID is
  invalid, or whose ID differs from the operand and binding IDs.

The contract imposes no selected-row cardinality: one function can emit a
selected restore for each applicable backward VLA goto.  It also does not
derive a target/block condition from text; the producer's existing typed HIR
control-flow condition is the selection source.

## Focused proof shape for Step 2

The nearest positive is a frontend-LIR VLA function with a resolved backward
goto: the emitted restore must have selected admission, the copied native
saved-pointer/object/owner/type/liveness binding, and the matching checkpoint
transition, then pass LIR verification.  Nearby negative mutations of that
same selected operation must independently reject missing admission, missing
transition, transition-ID mismatch, foreign owner, type mismatch, dead
authority, and a saved pointer not bound to its modeled current-function
definition.  These are producer/verifier tests only; this Step 1 note adds no
test and authorizes neither Raw-BIR/importer/734 work nor any other local or
lifetime row.
