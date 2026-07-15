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

### Step 1 - Publish the logical RHS conversion result allocation

Goal: replace only the non-`i1` RHS conversion result allocation in
`emit_logical` with a native `fresh_value` result.

Primary target:

- `src/codegen/lir/hir_to_lir/expr/binary.cpp`

Actions:

- preserve the existing typed RHS boolean operand and conversion type;
- allocate the RHS `LirCastOp.result` as `LirOperand::ssa` with an owning
  current-function ID;
- leave the raw PHI result/incoming and final logical consumer unchanged.

Completion check:

- accepted allocation fact: commit `3b716c12d` changes only the selected RHS
  conversion to use `fresh_value(ctx)` and retains the native result ID,
  without PHI, generic API, or other-family changes. Its focused proof is not
  complete: the old test assertion is expected to fail until Steps 2–3.

### Step 2 - Opt the selected logical RHS cast into native result authority

Goal: make only the selected RHS conversion subject to the existing standalone
cast-result verifier contract.

Primary target:

- `src/codegen/lir/hir_to_lir/expr/binary.cpp`

Actions:

- at construction of the selected non-`i1` RHS `LirCastOp`, set its existing
  `requires_native_result_authority` field to `true`;
- retain the `fresh_value(ctx)` result allocation from accepted commit
  `3b716c12d`;
- do not alter verifier or IR contracts: the existing flag-gated contract is
  already accepted and must remain the authority for missing, invalid,
  duplicate, and foreign IDs;
- do not touch PHI result/incoming, final logical consumer, generic expression
  APIs, or any other producer family.

Completion check:

- precisely the selected RHS cast opts in, so existing verifier ownership and
  duplicate/foreign checks require its native result authority.

### Step 3 - Prove the logical RHS result authority contract

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

### Step 4 - Publish the bounded 775 handoff

Goal: record the logical RHS field, proof, unresolved PHI boundary, and 775
return point without reactivating 775 or 751.

Completion check:

- 775 may consume this logical-RHS-only producer fact while PHI and other
  family work remain separate.

## Proof

- For Steps 2–3: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`
- The supervisor owns final regression and baseline acceptance; this runbook
  does not write root logs. A full-suite candidate is rejected until the
  obsolete assertion is repaired and it adds no failure.
