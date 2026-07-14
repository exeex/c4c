# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3.1
Current Step Title: Admit a native-floating CallSpec result

## Just Finished

- Plan Step 5.3 received the selected deferred i64 intrinsic-result `LirCastOp
  Trunc` row into a separately tagged Raw-BIR cast payload with an exact
  `i64 -> i32` edge, a source-backed owning result, and Raw/Canonical
  receipt-rejection coverage.
- The next selected fixed-void native-floating direct-call importer packet
  (contract committed in `aeecf048c`) built but its focused proof failed in
  `backend_lir_to_bir_interface`: `FunctionBuilder::append(BlockId, CallSpec)`
  rejects `spec.source_result_id && !integer_type(signature.return_type)` at
  `src/backend/bir/core/builder.cpp:1088-1091`, before importer acceptance.
  `frontend_lir_call_type_ref` passed. This durable text preserves the failure
  details; the matching `test_before.log` remains the prior green baseline.

## Suggested Next

- Execute only Plan Step 5.3.1: establish a source-backed native-floating
  result for the existing direct `CallSpec`, confined to resolved fixed-void
  `F32`/`F64` callees. Extend builder and reachable verifier coverage together:
  positive `F32`/`F64` result publication and negative void-with-result,
  result/signature mismatch, duplicate/cross-owner source identity, malformed
  result linkage, and rollback. Do not touch importer acceptance, FAdd, or any
  other floating-call form in this prerequisite.
- Exact return after acceptance: execute Plan Step 5.3.2, retrying the same
  `aeecf048c` resolved fixed-void native-floating `LirCallOp` importer packet.
  Its downstream double `LirBinOp FAdd` remains a source-chain guard only, not
  a receipt target.

## Watchouts

- This prerequisite changes only the native Raw-BIR `CallSpec` result gate. It
  does not admit an importer row by itself. Indirect, variadic, argument-
  bearing, ABI-expanded, unresolved, nonmatching coercion, aggregate/object,
  and every other floating-call form remain fail-closed. Scalar FAdd and all
  other binary/compare/select/abs rows, broader scalar casts, intrinsic
  alternatives, opaque inline assembly, text recovery, and partial module
  publication remain outside this packet.

## Proof

- Plan Step 5.3.1 delegated proof:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_lir_to_bir_interface$'`. The added cases in
  that suite must exercise both the builder and reachable Raw-BIR verifier.
  The supervisor must first prepare the matching canonical root baseline, then
  replace `test_after.log` with this prerequisite's matching after-proof.
- After Step 5.3.1 acceptance, Plan Step 5.3.2 reuses the interrupted proof:
  `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'
  > test_after.log`.
