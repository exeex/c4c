# LIR DirectScalar Body-Parameter Producer/Verifier Publication

Status: Open (active blocker for 734 Step 7.35)
Type: narrowly scoped codegen LIR producer/verifier correction
Blocked Parent: `ideas/open/734_lir_to_new_bir_container_completeness.md`,
Step 7.35

## Goal

Publish and validate the existing native `ull` DirectScalar current-function
parameter authority so producer-emitted, unselected DirectScalar rows do not
fail LIR verification before the selected 734 receiver row can run.

## Why This Exists

After the unaccepted 734 Step 7.35 receiver work, the matching backend proof
is 6/6 non-regressive, but the fresh external case
`llvm_gcc_c_torture_src_20041011_1_c` fails in `src/codegen/lir/verify.cpp`
before BIR import. The verifier rejects
`LirFunction.native_body_parameter_definitions` because the `ull` entry lacks
a native direct-pointer or direct-scalar current-function parameter identity
and type. This is a producer/emitter/verifier publication defect, not a
Raw-BIR importer defect.

## In Scope

- Trace the existing `ull` native DirectScalar body-parameter construction and
  its current-function identity/type path.
- Publish the typed native DirectScalar identity and type required by the
  existing `native_body_parameter_definitions` contract.
- Keep verification typed and fail-closed, including rejection of missing,
  foreign, type-incoherent, or unsupported direct-scalar authority.
- Add nearby producer/verifier coverage proving the selected `ull` shape
  verifies while malformed authority is rejected.
- Prove the repaired external case reaches past the prior LIR verifier failure.

## Out of Scope

- Any Raw-BIR type, builder, importer, dispatcher, or verifier change.
- Receiving generic BIR scalar parameters or any additional 734 body-parameter
  receiver row.
- Broad DirectScalar family redesign, ABI policy changes, parameter forms
  beyond the existing `ull` shape, or presentation-text recovery.
- Changes to the 734 selection: it remains only the selected DirectScalar
  `LirBinOp.lhs` receiver row.

## Acceptance Criteria

1. The existing `ull` DirectScalar producer path publishes a native
   current-function parameter identity and matching type sufficient for the
   typed LIR verifier contract.
2. Unselected DirectScalar rows no longer fail early solely because that
   authority was omitted; they are not thereby admitted to Raw-BIR receipt.
3. The LIR verifier remains fail-closed for malformed, foreign, absent, or
   type-incoherent DirectScalar authority.
4. A fresh build and focused producer/verifier proof pass, the exact external
   test `^llvm_gcc_c_torture_src_20041011_1_c$` no longer fails at the stated
   pre-import LIR verifier boundary, and the supervisor selects the matching
   regression/broader acceptance proof.
5. Handoff to 734 states that only its already selected DirectScalar
   `LirBinOp.lhs` receiver may resume; no generic scalar authority is granted.

## Reviewer Reject Signals

- Reject a BIR importer, Raw-BIR verifier, builder, or generic scalar-parameter
  change offered as the fix for this producer/verifier defect.
- Reject deriving identity or type from names, signatures, operands,
  diagnostics, rendered text, or testcase identity.
- Reject weakening `native_body_parameter_definitions` validation, accepting
  missing/foreign/type-incoherent authority, or replacing fail-closed behavior
  with a default scalar classification.
- Reject a named-case-only exception, expectation downgrade, or test rewrite
  that hides the `llvm_gcc_c_torture_src_20041011_1_c` verifier failure.
- Reject expansion to generic BIR scalar parameters, other ABI forms, or a
  broader DirectScalar authority family without a separately scoped idea.
