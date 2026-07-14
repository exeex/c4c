# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.3: received native integer intrinsic `LirCallOp` rows for
  `Cttz`, `Ctlz`, and `Ctpop` into a separate tagged Raw-BIR intrinsic-call
  payload with module `LinkNameId`, exact integer operands/signature, native
  zero-count behavior/`i1` flag where applicable, source-backed owning result,
  verifier coverage, and Raw/Canonical transactional rejection coverage.

## Suggested Next

- Execute the single selected Step 5.3 row: the deferred i64 intrinsic
  follow-on `LirCastOp Trunc` receipt only. This is receiver-ready because the
  handoff publishes the native result/source IDs and `Trunc` endpoints, and
  Step 5.3 now registers the i64 intrinsic producer result.

  - Typed source: one current-function `LirCastOp{result: LirValueId,
    operand: LirValueId, kind: LirCastKind::Trunc, from_type: i64,
    to_type: i32}` whose operand resolves to the just-supported i64
    `Cttz`/`Ctlz`/`Ctpop` intrinsic-call result. No immediate or text operand
    is admitted.
  - Typed destination: add one tagged Raw-BIR cast instruction payload with
    native `Trunc`, exact `i64 -> i32` endpoints, one source value edge, and
    one source-backed result-registry edge for the owning `LirValueId`; it must
    remain distinct from intrinsic-call identity and preserve the existing
    intrinsic result as its operand.
  - Import/verifier obligations: resolve the operand only in the current
    function; require unique result definition, exact `Trunc` kind/endpoints,
    a compatible registered i64 intrinsic result, and an i32 result. Verify
    Raw-BIR payload/tag/arity/type/result-use coherence and reject atomically
    for missing or duplicate result IDs, unknown/foreign/non-intrinsic source,
    wrong kind/endpoints, type conflict, malformed payload, or every other
    cast/operand alternative.
  - Focused proof: add Raw and Canonical intrinsic-to-Trunc receipt assertions
    plus transactional rejection coverage in `backend_lir_to_bir_interface`;
    retain the producer/verifier chain in `frontend_lir_call_type_ref`. The
    delegated proof is `cmake --build --preset default` then
    `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.

## Watchouts

- This packet admits only the i64 intrinsic-result to i32 `Trunc` edge.
  Broader scalar casts (integer width changes, FPTrunc/FPExt,
  SIToFP/UIToFP, FPToSI/FPToUI), scalar binary/compare/select/abs rows, direct
  floating calls, and opaque inline assembly remain separately scoped. Other
  intrinsic kinds and unsupported direct/indirect/variadic/ABI/aggregate/object
  forms remain fail-closed; no text recovery or partial module publication.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$' > test_after.log`
  passed for Step 5.3; `test_after.log` is the preserved proof log.
