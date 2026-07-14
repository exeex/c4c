# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.3: received the selected deferred i64 intrinsic-result
  `LirCastOp Trunc` row into a separately tagged Raw-BIR cast payload with an
  exact `i64 -> i32` edge, the current-function `Cttz`/`Ctlz`/`Ctpop` result
  as its sole operand, and a source-backed owning result. The builder,
  importer, and verifier fail closed for every other cast shape, operand
  authority, result collision, foreign/non-intrinsic source, or malformed
  Raw-BIR payload; Raw and Canonical receipt/rejection coverage was added.

## Suggested Next

- Execute only the next ordered Step 5.3 handoff row: a resolved fixed-void
  native-floating `LirCallOp` whose `result: LirValueId` is owned by the
  current function, whose direct global `callee: LinkNameId` resolves to one
  module Function, and whose `return_type` and
  `callee_signature.return_type_ref` are the same native floating type with an
  empty fixed-void structured signature. The exact result must be the lhs
  `LirValueId` of the downstream double `LirBinOp FAdd`; that FAdd establishes
  the checked source use chain but is not itself a scalar-binary receipt in
  this packet.

  - Typed Raw-BIR destination intent: receive one tagged resolved-direct-call
    payload for that zero-argument native-floating Function call, preserving
    the declaration/callee `LinkNameId`, matching floating return/signature,
    and one source-backed current-function result-registry definition. Retain
    the result identity so the later FAdd receiver row can resolve its lhs;
    do not create a Raw-BIR FAdd payload here.
  - Import/verifier contract: resolve exactly one module Function by the
    direct `LinkNameId`; require matching fixed-void signature, native
    floating return, and one owning unique result definition. The retained LIR
    source-verifier chain must require that result as the downstream double
    FAdd lhs with no unknown/cross-function use or type conflict; the Raw-BIR
    verifier must require tagged call payload/signature/result coherence.
    Reject transactionally for missing or duplicate declaration/result identity,
    signature/return/FAdd type conflict, malformed payload, or any alternative
    carrier.
  - Focused proof target: extend Raw and Canonical receipt/rejection coverage
    in `backend_lir_to_bir_interface`, while retaining
    `test_block_scope_extern_void_prototype_uses_direct_function_entity` in
    `frontend_lir_call_type_ref` as the producer/verifier source-chain guard.
    The delegated proof remains `cmake --build --preset default` followed by
    `ctest --test-dir build -j --output-on-failure -R
    '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.

## Watchouts

- This packet admits only the resolved, direct, zero-argument fixed-void
  native-floating Function-call subrow and its typed result identity. Indirect,
  variadic, argument-bearing floating, ABI-expanded, unresolved, nonmatching
  coercion, aggregate/object, and every other floating-call form remain
  fail-closed. Scalar FAdd and all other binary/compare/select/abs rows,
  broader scalar casts, intrinsic alternatives, opaque inline assembly, text
  recovery, and partial module publication remain outside this packet.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$' > test_after.log`
  passed for this Step 5.3 packet; `test_after.log` is the preserved proof log.
