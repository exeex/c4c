# Local Scalar Store Authority Handoff To Idea 734

This bounded Step 1 handoff from idea 790 authorizes one Raw-BIR receiver row
only. The earlier direct local-scalar load is accepted historical work.

## Selected variant and native contract

The sole authorized variant is the direct, non-array, non-VLA integer
`LirStoreOp` emitted for a local scalar declaration initialized from a native
integer immediate. Its receiver inputs are native fields, never operand text:

| Field | Required fact |
| --- | --- |
| `requires_native_store_authority` | True for this selected producer route. |
| `type_str` | Exact integer `LirTypeRef` of the stored scalar. |
| `val` | Native representable `LirIntegerImmediate`. |
| `ptr` | Valid `LirValueId` equal to `local_object_authority.pointer_definition`. |
| `local_object_authority` | Valid/live current-function `object`, `owner`, pointer definition/type, and pointee type. |
| type coherence | `local_object_authority.pointee_type == type_str`. |

The verifier checks selected-store admission, native immediate representability,
matching pointee/store type, and the existing current-function
pointer/object/owner/liveness binding. Presentation is nonsemantic: a receiver
must not recover or validate facts through a local name, `%t`, rendered operand,
printer output, LLVM text, or testcase identity.

## Guarantees and rejected forms

The receiver may consume only the documented immediate, type, pointer
definition, and checked local-object authority transactionally. Missing local
authority, raw/nonrepresentable value, invalid pointer, foreign or incoherent
object/owner, type mismatch, and dead authority reject before downstream use.

All assignment stores, SSA-valued stores, pointer/aggregate/vector stores,
array/VLA stores, every GEP and later load, stack-save/restore, and all other
local-operation families remain unsupported and fail closed.

## Evidence

- Producer/schema/verifier selection: current idea 790 Step 1 worktree.
- Focused proof: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`.

## Exact Step 7.28 return action

Resume 734 at **Step 7.28 - Receive the selected direct local-scalar
`LirStoreOp` authority**. Add only the minimum typed Raw-BIR destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage for these native fields. Do not repeat the accepted alloca or local
load receipts and do not use presentation recovery.
