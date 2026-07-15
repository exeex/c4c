# Current Packet

Status: Active
Source Idea Path: ideas/open/789_lir_local_operation_receiver_handoff_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify and prove the selected producer boundary

## Just Finished

- Step 1 selected exactly one first post-alloca producer row: the direct,
  non-array/non-VLA local-scalar `LirLoadOp` emitted for a local `DeclRef`
  rvalue by `emit_decl_ref_rval_operand`.  It is selected by native facts only:
  `result` is the fresh SSA `LirValueId`, `type_str` is its `LirTypeRef`, and
  `ptr` is the SSA operand whose `LirValueId` equals
  `local_object_authority.pointer_definition`.  The required authority fields
  are `pointer_definition`, valid `object`, current-function `owner`, pointer
  `pointer_type`, `pointee_type`, and `live`; the receiver-relevant operation
  fields are `result`, `type_str`, and `ptr`.
- Existing verification already rejects absent/invalid/foreign/dead authority
  and binds `ptr` to a current-function pointer definition with canonical
  object facts, but it currently passes no expected pointee type for a load.
  Step 2 must prove this selected load's `pointee_type == type_str`, its native
  result identity, and the matching malformed rejections before any 734
  handoff can be claimed.

## Suggested Next

- Execute Step 2 only: tighten/prove the selected direct local-scalar
  `LirLoadOp` contract; do not publish a 734 handoff yet.

## Watchouts

- Every other row remains excluded: all other direct/access/array/VLA local
  loads; all local stores (including initializer, assignment, and VLA
  pointer-slot stores); every local GEP (whose element/index contract is
  separate); and VLA stack-save/restore and dynamic-allocation lifetime rows.
  Their value, index, element-type, dynamic-object, or lifetime contracts need
  their own bounded selection.
- Do not change Raw-BIR/importer code or select semantics from local names,
  `%t`, formatted operands, printer output, LLVM text, or testcase shape.

## Proof

- Passed current-state check: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1);
  `test_after.log`. This confirms the current producer/verifier state only,
  not Step 2's selected-load proof or a 734 handoff.
