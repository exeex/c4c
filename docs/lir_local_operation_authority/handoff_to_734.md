# Local Scalar Load Authority Handoff To Idea 734

This is the bounded Step 3 handoff from closed idea 789 to
`ideas/open/734_lir_to_new_bir_container_completeness.md`. It authorizes one
Raw-BIR receiver packet only; it does not claim that Raw-BIR already receives
this row.

## Selected variant and native contract

The sole authorized variant is the direct, non-array, non-VLA local-scalar
`LirLoadOp` emitted by `StmtEmitter::emit_decl_ref_rval_operand` for a local
`DeclRef` rvalue. Its receiver inputs are native fields, not operand text:

| Field | Required fact |
| --- | --- |
| `result` | Valid `LirValueId` for the load result. |
| `type_str` | The load's `LirTypeRef`. |
| `ptr` | A value ID equal to `local_object_authority.pointer_definition`. |
| `local_object_authority` | Valid current-function local object: `object`, `owner`, `pointer_definition`, `pointer_type`, `pointee_type`, and `live`. |
| type coherence | `local_object_authority.pointee_type == type_str`. |

`requires_native_result_authority` is true for this selected producer route.
The LIR verifier admits it only with that admission bit, a valid native result,
the authority pointer bound to `ptr`, and the exact pointee/load-type equality.
The existing authority verifier additionally enforces current-function object
ownership, pointer/object/type coherence, and liveness.

## Guarantees and rejected forms

The receiver may consume only the selected load result, load type, pointer
definition, and checked local-object authority transactionally. It must not
recover result, pointer, object, type, or owner from a local name, `%t`,
formatted operand, printer output, LLVM text, or testcase identity.

The producer/verifier rejects missing or invalid authority; foreign or dead
authority; pointer, object, or display disagreement; missing/invalid native
result; disabled native-result admission; and load-pointee/type mismatch.

All other direct/access/array/VLA loads; every store; every GEP; VLA
stack-save/restore and dynamic-allocation lifetime; named/local-temporary
variants; Raw-BIR work outside this receiver packet; and every later family
remain unsupported and fail closed.

## Evidence

- Producer selection: `d48236bd9`.
- Producer/verifier implementation and focused same-feature coverage:
  `d290ffc21`.
- Fresh acceptance: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1.
- Matching canonical `test_before.log` / `test_after.log` regression guard
  passed non-decreasing.

## Exact Step 7.27 return action

Resume 734 at **Step 7.27 - Receive the selected direct local-scalar
`LirLoadOp` authority**. Add only the minimum typed Raw-BIR destination,
importer dispatch, reachable verification, and transactional positive/negative
coverage for the fields above. Do not repeat Step 7.26's accepted alloca
receipt and do not use presentation recovery.
