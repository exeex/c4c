# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.3
Current Step Title: Take subsequent checked ordinary rows one at a time

## Just Finished

- Plan Step 5.2: received resolved direct integer-result `LirCallOp` rows into
  Raw-BIR with owning source-value results, typed direct callee/signature/
  argument edges, immediate or same-function SSA arguments, verifier reachability,
  and transactional positive/negative receipt coverage.

## Suggested Next

- Execute the single selected Step 5.3 row: native integer intrinsic
  `LirCallOp` receipt for `LirIntrinsicKind::{Cttz,Ctlz,Ctpop}` only.

  - Typed source: one current-function `LirCallOp.result: LirValueId`; intrinsic
    `callee`/`direct_callee_link_name_id: LinkNameId`; integer
    `return_type` plus matching fixed structured signature/type refs; and one
    current-function `LirValueId` or representable integer-immediate value
    operand. `Cttz`/`Ctlz` additionally carry exactly one native `i1`
    immediate flag and `LirZeroCountBehavior::{Defined,Undefined}`;
    `Ctpop` carries no zero-count behavior.
  - Typed destination: add one separately tagged Raw-BIR intrinsic-call
    instruction payload (not a direct-`FunctionId` `CallNode`) that retains
    the native intrinsic kind, module `LinkNameId` identity, integer
    signature/result type, ordered value operand(s), `Cttz`/`Ctlz`
    zero-count behavior and matching `i1` flag, and a source-backed result
    registry edge for the owning `LirValueId`.
  - Import/verifier obligations: resolve the intrinsic link through the
    module link registry; require exactly the native nonvariadic one-value
    form (`Ctpop`) or value-plus-`i1` flag form (`Cttz`/`Ctlz`), exact integer
    type/signature/argument agreement, result uniqueness and current-function
    ownership, representable immediate or same-function SSA operands, and
    kind/behavior/flag consistency. Reject atomically on missing or duplicate
    result/link identity, wrong signature/count/type, foreign or unknown SSA,
    invalid immediate, unsupported operand alternative, behavior/flag mismatch,
    or malformed Raw-BIR payload.
  - Focused proof: add Raw and Canonical receipt assertions plus transactional
    rejection coverage in `backend_lir_to_bir_interface`; retain the producer
    contract with `frontend_lir_call_type_ref`. The delegated proof is
    `cmake --build --preset default` then
    `ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_interface|frontend_lir_call_type_ref)$'`.

## Watchouts

- Direct-call receipt remains restricted to resolved `LinkNameId`, integer result,
  fixed nonvariadic exact signatures, and immediate/current-function SSA arguments.
  Indirect, variadic, ABI-expanded, aggregate/object, unresolved, coercing, and
  floating forms remain unsupported without text recovery.
- This packet does not receive the i64 intrinsic follow-on `LirCastOp Trunc`,
  scalar binary/compare/select/abs rows, direct floating calls, or opaque
  inline-assembly rows. It also rejects every other intrinsic kind, text-derived
  identity/semantics, indirect/variadic/ABI/aggregate/object forms, and all
  unsupported operand alternatives; no partial module may publish.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  for Step 5.2; `test_after.log` is the preserved proof log.
