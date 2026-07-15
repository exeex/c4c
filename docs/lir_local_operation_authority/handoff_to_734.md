# Local Operation Authority Handoff To Idea 734

This bounded Steps 1–3 handoff from idea 791 authorizes one producer-backed
LIR receiver row only. The accepted alloca, local load, and declaration-store
receipts remain historical work.

## Selected variant and native contract

The sole authorized variant is the direct static-local-array `LirGepOp` emitted
for an integer-immediate index. Its receiver inputs are native fields, never
operand text:

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

## Guarantees and rejected forms

The receiver may consume only the documented result, element type, base pointer,
immediate index, and checked local-object authority transactionally. Missing
admission/local authority, raw or SSA index, invalid result/base, foreign or
incoherent object/owner, type mismatch, and dead authority reject before
downstream use.

All stores, SSA-indexed/local-temporary/aggregate-member GEPs, VLA GEPs, every
later load, stack-save/restore, and all other local-operation families remain
unsupported and fail closed.

## Evidence

- Producer/schema/verifier selection: current idea 791 Steps 1–3 worktree.
- Focused proof: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`.

## Exact Step 7.29 return action

Resume 734 at **Step 7.29 - Receive the selected direct static-local-array
`LirGepOp` authority**. Add only the minimum typed Raw-BIR destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage for these native fields. Do not repeat the accepted alloca, local-load,
or declaration-store receipts and do not use presentation recovery.
