# LIR Next Body Parameter Authority Handoff

Status: Open
Type: producer/schema/verifier handoff for one next function-body parameter-use row
Parent Source: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Trace, publish, verify, and hand off exactly one next valid current-LIR
function-body parameter-use authority row after accepted 734 Step 7.48.

## Why This Exists

Idea 734 can receive only structured LIR authority into typed Raw BIR. Its
latest accepted receiver packet, commit `6a91d07ca`, consumed the closed-859
DirectScalar binary-`fsub` LHS row. The 734 completion gate remains unmet, but
no further receiver row is authorized without a separately accepted
producer-side handoff.

## In Scope

- Inspect current LIR body-parameter producer and verifier behavior after the
  accepted DirectPointer and DirectScalar receipts through 734 Step 7.48.
- Select exactly one next valid function-body parameter-use row that current
  LIR can publish with native structured authority.
- Add only the LIR producer/schema/verifier fields and checks required for
  that one selected row.
- Verify the row's parameter value identity, owner, parameter index, typed
  `LirTypeRef`, native body-parameter ABI, explicit role, and selected
  consumer relation.
- Add focused malformed-authority coverage for absent, invalid, duplicate,
  foreign, owner/index/type/ABI/role-incoherent, and consumer-incoherent
  forms relevant to the selected row.
- Hand off the exact row, fields, proof, and malformed matrix back to 734 for
  one later Raw-BIR receiver packet.

## Out Of Scope

- Raw-BIR containers, builders, views, importer dispatch, verifier, or backend
  receiver tests.
- Reopening any accepted 734 body-parameter receiver row, including
  DirectPointer GEP/truthiness rows and DirectScalar binary, unary, return,
  switch, comparison, direct-call, `fmul`, `fadd`, or `fsub` rows.
- Generic parameter sweeps, ABI-expanded or aggregate parameter families,
  memory/VA, aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, inline-assembly, or target-lowering work.
- Recovering authority from text, names, rendered operands, signatures,
  diagnostics, compatibility mirrors, `monostate`, or testcase shape.

## Acceptance Criteria

- Exactly one next valid body-parameter-use row is selected and documented.
- The selected row has native structured LIR authority for the parameter tuple
  and selected consumer relation.
- The LIR verifier rejects malformed authority before downstream use.
- Focused producer/verifier coverage proves the positive row and nearby
  malformed matrix.
- The handoff names the exact 734 return action and states which rows remain
  unsupported or separately scoped.
- Supervisor-selected focused proof and any required matching regression guard
  are accepted.

## Reviewer Reject Signals

- Reject any Raw-BIR/importer/container/verifier receiver edit; that work
  belongs to 734 after this handoff closes.
- Reject selecting multiple rows, a generic body-parameter family, or any row
  already accepted by 734 through Step 7.48.
- Reject recovering parameter identity, role, type, opcode, operand relation,
  or consumer coherence from rendered text, names, signatures, diagnostics,
  compatibility mirrors, `monostate`, or testcase-specific shape.
- Reject expectation downgrades, unsupported-to-supported label changes,
  helper renames, or classification-only edits claimed as authority
  publication.
- Reject broad LIR schema churn, ABI redesign, target-lowering behavior, or
  unrelated memory/VA, aggregate/vector, module/type/global/metadata,
  instruction/terminator, or inline-assembly work.

## Resumption Record - Interrupted For Baseline Blocker 861

Last accepted progress: none for this idea's Step 1. No producer-side next-row
selection or implementation has been accepted under idea 860.

Completed runbook steps: none.

Interrupted step:

- Step ID: `1`
- Step Title: `Select and publish one next body-parameter authority row`

Blocker: `ideas/open/861_lir_scalar_lhs_parameter_authority_baseline_repair.md`
must repair the current-LIR scalar LHS parameter authority verifier/producer
failure family before 860 can accept any implementation commit.

Why outside 860 scope: the full-suite baseline fails at current `HEAD` before
any 860 implementation. The failure reopens the already accepted DirectScalar
row contract by reporting
`LirBinOp.scalar_lhs_parameter_authority: selected floating binary LHS authority
requires a nonselected scalar RHS`. Idea 860 is scoped to selecting and
publishing one next body-parameter authority row, and explicitly must not
reopen accepted DirectScalar body-parameter receiver rows.

Exact return point: after blocker 861 closes and the supervisor accepts a green
baseline, resume this idea at Step 1, `Select and publish one next
body-parameter authority row`.

Remaining next action: select exactly one next valid current-LIR
body-parameter-use row after the accepted DirectScalar rows, then implement
only that producer/schema/verifier/test handoff.

Accepted proof references for 860 Step 1: none.

Baseline failure evidence preserved for the blocker:

- Fresh `HEAD` build had no work to do.
- Fresh full suite failed with `99% tests passed, 12 tests failed out of 3038`.
- Failure family:
  `error: LirBinOp.scalar_lhs_parameter_authority: selected floating binary LHS
  authority requires a nonselected scalar RHS`.
- Failed tests:
  `cpp_positive_sema_specialization_identity_cpp`,
  `cpp_llvm_spec_key_metadata`, `cpp_llvm_spec_key_named_metadata`,
  `cpp_llvm_spec_key_named_metadata_entry`,
  `llvm_gcc_c_torture_src_20020314_1_c`,
  `llvm_gcc_c_torture_src_921208_1_c`,
  `llvm_gcc_c_torture_src_990127_2_c`,
  `llvm_gcc_c_torture_src_990829_1_c`,
  `llvm_gcc_c_torture_src_cbrt_c`,
  `llvm_gcc_c_torture_src_conversion_c`,
  `llvm_gcc_c_torture_src_gofast_c`, and
  `llvm_gcc_c_torture_src_ieee_unsafe_fp_assoc_1_c`.
- `test_baseline.log` is green at `380ee782...`.
- `test_baseline.new.log` failed at historical commit `a23031c8...` with the
  same family plus two extra failures.
- Supervisor rejected the candidate via
  `scripts/plan_review_state.py reject-baseline`.

## Closure Record

Disposition: capability complete for this bounded producer-side handoff.

Accepted implementation: commit `ed64ebcf1` (`Publish fsub rhs parameter
authority`).

Selected row: exactly DirectScalar floating binary `fsub` RHS for fixture
`double scalar_fsub_rhs(double x) { return 2.0 - x; }`.

Accepted authority: current LIR now publishes native structured
`LirScalarBinaryRhsParameterAuthority` for the selected RHS parameter tuple and
consumer relation. The verifier rejects malformed authority before downstream
use, with focused absent, invalid, duplicate, foreign owner, owner/index/type,
ABI, role, and consumer-incoherent coverage for the selected row.

Accepted proof:

- Focused proof passed:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'; } > test_after.log 2>&1`
- Full suite passed:
  `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`
- Full-suite result: `3038/3038`.
- `git diff --check` passed before commit.
- Focused before/after guard was not applicable because the focused subset was
  already green before and after; the checker reported no strict pass-count
  increase rather than a new failure.

Raw-BIR receiver status: no Raw-BIR container, importer, verifier, backend
receiver, or receiver-test work landed under this idea.

Handoff back to 734: resume parent
`ideas/open/734_lir_to_new_bir_container_completeness.md` with one later
Raw-BIR receiver packet for the DirectScalar floating binary `fsub` RHS
authority published here. That packet should receive exactly this RHS
parameter authority into typed Raw BIR and keep all other rows unsupported or
separately scoped unless their own producer handoff has closed.
