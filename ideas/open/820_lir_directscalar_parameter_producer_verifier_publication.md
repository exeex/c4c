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

## Resumption Record: independent frontend LIR switch-fixture blocker

- Last accepted progress: Step 1 trace was accepted in `35cff0993`. Step 2,
  *Publish and verify the typed DirectScalar authority*, was accepted in
  `4bcc7c8ff`: the existing `ull` producer publishes current-function native
  DirectScalar identity/type, and focused positive plus missing, foreign, and
  type-incoherent authority coverage protects the fail-closed verifier
  boundary.
- Interrupted step: Step 3, *Prove the boundary and record the 734 handoff*,
  remains incomplete. Its focused broader probe stopped before any
  DirectScalar test at the manually constructed `frontend_lir_call_type_ref`
  switch fixture: `LirSwitch.selector: must identify a current-function
  integer value definition`.
- Blocker and scope boundary: the fixture/modelled switch-result authority
  correction is independent of `4bcc7c8ff` (the fixture and selector verifier
  branch are unchanged from `4bcc7c8ff^`) and is outside this DirectScalar
  producer/verifier idea. It is owned by
  `ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md`.
  That blocker must not change DirectScalar, Raw-BIR, importer, generic scalar
  receipt, or generic switch feature scope.
- Exact return point: after 821 has supervisor-accepted focused proof, resume
  this idea at unchanged Step 3. Run the fresh build, focused
  producer/verifier proof, the exact
  `^llvm_gcc_c_torture_src_20041011_1_c$` test, and the supervisor-selected
  matching broader checkpoint. Record only the existing selected 734
  `LirBinOp.lhs` handoff; do not claim Step 3 complete before that checkpoint
  succeeds.
- Accepted proof references: the exact external gcc-torture pass and matching
  exact before/after regression guard accepted with `4bcc7c8ff`; the current
  `frontend_lir_call_type_ref` failure is diagnosis only and is not acceptance
  proof for Step 3.

## Runbook Repair Decision: Step 3 composite-proof boundary

Close and handoff are rejected. Step 3 remains incomplete despite a fresh
`cmake --build --preset default --clean-first` and a passing exact
`^llvm_gcc_c_torture_src_20041011_1_c$` CTest in the protected composite
worktree. The narrow `backend_lir_selected_pointer_authority` proof passed
before that clean build. Those results are causal evidence that the former
DirectScalar boundary is passed in the composite, but they are not acceptance
evidence for this source: the external pass depends on unaccepted out-of-scope
Ideas 821/822 selector patches and unaccepted 734 Step 7.35-shaped Raw-BIR
receiver work.

The attempted broader `^backend_` CTest cannot repair that gap: after the
clean build, six registered executables are missing, making that command
infrastructure-invalid rather than a semantic pass or fail. The frontend
focused test remains red at the existing DirectScalar authority abort; its
strict-count comparison is non-accepting diagnostic evidence.

Classification: `repair-current-route`. Keep this idea active at unchanged
Step 3, preserve every pending shared-worktree patch, and select an executable
matching checkpoint whose result is attributable to accepted prerequisites and
the accepted `4bcc7c8ff` DirectScalar work. Only after that accepted checkpoint
may this source record the narrow 734 handoff. The exact return point is Step
3, *Prove the boundary and record the 734 handoff*; do not activate or accept
734 Step 7.35 from the current composite evidence, and grant no generic scalar
authorization.
