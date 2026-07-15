# Current Packet

Status: Active
Source Idea Path: ideas/open/789_lir_local_operation_receiver_handoff_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish the bounded 734 handoff

## Just Finished

- Step 2 verified and proved the selected direct non-array/non-VLA local-scalar
  `LirLoadOp` only. `emit_decl_ref_rval_operand` now marks the load as requiring
  native result authority; its verifier requires a valid `result` `LirValueId`,
  requires the authority pointer to equal `ptr`, and binds
  `local_object_authority.pointee_type == type_str`. Nearby coverage proves the
  positive native result/pointer/type facts and rejects missing native-result
  admission, invalid result identity, and mismatched load pointee type. Existing
  local-authority coverage retains missing/invalid/foreign/dead, pointer, object,
  and display-mismatch rejection.

## Suggested Next

- Execute Step 3 only: publish this one verified local-scalar-load contract for
  734 Step 7.27; do not select any other local operation family.

## Watchouts

- Every other row remains excluded: all other direct/access/array/VLA local
  loads; all local stores (including initializer, assignment, and VLA
  pointer-slot stores); every local GEP; and VLA stack-save/restore and dynamic
  allocation lifetime rows. Do not widen 734's return action beyond the one
  selected operation or derive semantics from display text.

## Proof

- Passed Step 2 proof: `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`
  (1/1); `test_after.log`.
