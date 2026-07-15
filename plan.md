# LIR Logical RHS Result Authority Publication Runbook

Status: Active
Source Idea: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Resumed from: closed 779's accepted standalone cast-result authority prerequisite.

## Goal

Publish one native logical RHS conversion result ID without migrating generic
expression APIs or changing the downstream PHI receiver.

## Core Rule

Allocate authority with `fresh_value` before rendering; raw display text must
not be used to create or recover a value ID.

## Read First

- `ideas/open/778_lir_logical_rhs_result_authority_publication.md`
- `ideas/closed/779_lir_cast_result_authority_contract.md`
- `ideas/closed/776_lir_typed_expression_result_carrier_decomposition.md`
- `ideas/open/775_lir_phi_producer_helper_result_identity.md`
- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `src/codegen/lir/hir_to_lir/expr/binary.cpp`
- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

## Non-Goals

- no logical PHI/incoming/result carrier work or final logical consumer claim
- no ternary/coerce, vaarg, or generic expression migration
- no text recovery, maps, side tables, Raw-BIR/importer, backend, target
  lowering, MIR, or emission work

## Ordered Steps

### Step 1 - Publish the logical RHS conversion result

Goal: replace only the non-`i1` RHS conversion result allocation in
`emit_logical` with a native `fresh_value` result.

Primary target:

- `src/codegen/lir/hir_to_lir/expr/binary.cpp`

Actions:

- make a clean reattempt; do not reuse the prior unaccepted local diff;
- preserve the existing typed RHS boolean operand and conversion type;
- allocate the RHS `LirCastOp.result` as `LirOperand::ssa` with an owning
  current-function ID;
- leave the raw PHI result/incoming and final logical consumer unchanged.

Completion check:

- the selected RHS conversion produces a valid native result ID without PHI,
  generic API, or other-family changes.

### Step 2 - Prove the logical RHS result authority contract

Goal: add focused positive and fail-closed malformed proof for the selected
RHS conversion result only, using the accepted standalone-cast verifier
contract from closed 779.

Primary target:

- `tests/frontend/frontend_lir_call_type_ref_test.cpp`

Actions:

- require the RHS `LirCastOp.result` native ID without rendered-text or order
  matching;
- prove missing, invalid, duplicate, and foreign RHS-result authority rejects;
- keep PHI and final logical consumer expectations explicitly outside this
  packet.

Completion check:

- focused logical RHS result proof passes and malformed authority fails closed
  through the accepted verifier contract.

### Step 3 - Publish the bounded 775 handoff

Goal: record the logical RHS field, proof, unresolved PHI boundary, and 775
return point without reactivating 775 or 751.

Completion check:

- 775 may consume this logical-RHS-only producer fact while PHI and other
  family work remain separate.

## Proof

- For Steps 1–2: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`
- The supervisor owns final regression and baseline acceptance; this runbook
  does not write root logs.
