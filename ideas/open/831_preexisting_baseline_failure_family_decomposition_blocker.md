# Pre-existing Baseline Failure-Family Decomposition Blocker

Status: Open
Type: bounded baseline provenance and first-owner decomposition prerequisite
Blocked Parent: `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`, Step 3

## Goal

Classify the independently pre-existing `frontend_hir_tests` segfault and
`LirCmpOp.truthiness_lhs_parameter_authority` torture failures by first owning
layer, then create the smallest separately scoped repair route(s) needed to
restore a comparable full-suite baseline for 830.

## Why This Exists

830's post-commit candidate is rejected (3024/3038 versus accepted 3038/3038),
but evidence shows both failure families predate its direct-call argument-1
slice. The HIR segfault reproduces from a clean isolated `f0fc85e4f^` build
with `ENABLE_C4C_BACKEND=ON`; all 13 torture failures use the unchanged
`LirCmpOp.truthiness_lhs_parameter_authority` route. The families have no
shared implementation seam, so combining their repairs would hide ownership
and improperly widen 830.

## Current Evidence

- Accepted `test_baseline.log` at `8418036b`: 3038/3038.
- Rejected `test_baseline.new.log` at `f0fc85e4f`: 3024/3038.
- Exact reproduction command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|llvm_gcc_c_torture_src_(20090113_2|930719_1|931012_1|950512_1|961112_1|comp_goto_1|pr23604|pr28289|pr37780|pr43385|pr46909_2|pr51323|pr88714)_c)$' > test_after.log`.
- One `frontend_hir_tests` segfault and 13 named torture failures, all from
  `LirCmpOp.truthiness_lhs_parameter_authority`.

## In Scope

- Reproduce and reduce each family only far enough to identify its first
  owning layer and confirm the supplied pre-830 provenance.
- Record whether a common owning seam exists; if not, create ordered,
  separately scoped repair ideas, one for the HIR segfault and one for the
  truthiness-parameter verifier family.
- After the repair routes have accepted focused proof, require a comparable
  full-suite baseline proving no new baseline problem before returning 830 to
  its unchanged Step 3 acceptance gate.

## Out Of Scope

- Any direct-call argument identity/type construction, 829 authority work,
  Raw-BIR, generic call work, or 830 code change.
- Repairing both families in one mixed implementation packet, unless new
  evidence demonstrates one concrete shared first-owner seam.
- Test expectation changes, unsupported-marker/allowlist edits, test filtering,
  or weaker harness contracts as baseline clearance.

## Acceptance Criteria

1. The first diagnostic packet preserves the exact two-family reproduction and
   states whether a shared first-owner seam exists.
2. If no shared seam exists, each repair family has a separately scoped open
   successor with an explicit owning layer and dependency order; no mixed
   implementation route is authorized by this idea.
3. The selected repair route(s) obtain accepted focused proof and an accepted
   comparable full-suite result that clears the 3038/3038 baseline problem.
4. The durable completion record returns only 830 to unchanged Step 3, then
   its existing Step 4 and 829 Step 2 continuation.

## Reviewer Reject Signals

- Reject a combined HIR-segfault/truthiness-verifier code patch without
  evidence of one shared first-owner seam.
- Reject attributing either family to `f0fc85e4f` without disproving the
  supplied parent-revision reproductions.
- Reject testcase-shaped verifier exceptions, expectation downgrades,
  unsupported markers, allowlists, test filtering, or weaker harness behavior
  presented as repair or baseline clearance.
- Reject any 830 direct-call argument-1, 829 authority, Raw-BIR, generic-call,
  or presentation-derived change as part of this blocker.
- Reject closing or returning to 830 from focused proof alone without accepted
  comparable full-suite evidence.

## Resumption Record: ordered repair successors after Step 1

Status: parked by lifecycle switch to
`ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md`.

- Last accepted progress: Step 1 is complete, recorded in `ba7958ee4`.
  Fresh exact-subset proof failed 14/14, preserving one `frontend_hir_tests`
  segfault and the 13 truthiness-authority rejections.
- Completed step: Step 1 — **Establish first-owner decomposition from exact
  reproductions**.
- Decomposition result: the HIR crash first owns at
  `typespec_aggregate_owner_key`, reached through `lir_owned_type_spec` and
  `populate_lir_function_params` in
  `test_hir_to_lir_object_helper_callees_prefer_link_name_ids`; the separate
  torture family first owns at unchanged
  `verify_truthiness_lhs_parameter_authority`, missing native
  `LirCmpOp.truthiness_lhs_parameter_authority` for a direct-scalar truthiness
  LHS. A clean backend-enabled `f0fc85e4f^` reproduces the HIR crash. No common
  implementation seam exists.
- Ordered successors: first
  `ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md`,
  then
  `ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md`.
  The routes must not be combined.
- Exact return point: after 832 completes accepted focused proof, reactivate
  831 solely to switch/activate 833. After 833 completes, reactivate 831 at
  Step 2 to collect both accepted proofs, then execute Step 3 comparable full
  suite and return 830 unchanged at Step 3. Do not claim baseline clearance
  until that full-suite gate is accepted.

## Resumption Update: closed 832 and active ordered 833 successor

- 832 is capability-complete for the bounded HIR aggregate-owner crash and is
  archived at
  `ideas/closed/832_hir_aggregate_owner_function_parameter_crash_repair.md`.
  Accepted repair `b556c6c9f`, matching 0/1-to-1/1 focused guard, checker, and
  Step 3 record `b31cfa6ec` establish only that family; no full baseline pass
  is claimed.
- This lifecycle return immediately switches to ordered open successor
  `ideas/open/833_lir_truthiness_lhs_parameter_authority_completion.md` at
  its Step 1 direct-scalar truthiness-LHS authority diagnosis packet.
- Exact next return: after 833 has accepted focused proof, reactivate 831 at
  Step 2 solely to collect 832 and 833 evidence, then run Step 3 comparable
  full-suite proof before returning 830 unchanged at Step 3.

## Resumption Update: closed 833 and resumed Step 2 evidence collection

- 833 is capability-complete for its bounded native direct-scalar
  truthiness-LHS authority relation and is archived at
  `ideas/closed/833_lir_truthiness_lhs_parameter_authority_completion.md`.
  Its completed Steps 1, 2a, 2b, and 3 establish the actual binary-`Ne`
  producer seam, native authority population, generated-path and malformed
  verifier coverage, and accepted focused proof.
- Accepted implementation and proof: `9b5046952`, fresh build, and matching
  canonical exact 13-case regression logs accepted by the supervisor guard:
  0/13 before to 9/13 after, with zero new failures. This establishes only
  that the nine repaired cases no longer stop at the missing
  `LirCmpOp.truthiness_lhs_parameter_authority` relation.
- Residual ownership: `20090113-2`, `comp-goto-1`, `pr51323`, and `pr88714`
  remain visible as aggregate-owner family errors outside 833. They are not
  baseline clearance and do not revise the ordered-family provenance.
- Exact resumed return point: Step 2 — **Collect accepted successor evidence
  and confirm family boundaries**. Record the accepted 832 and 833 focused
  proofs and their remaining-family boundaries, then execute Step 3's
  comparable full-suite baseline gate before returning 830 unchanged at Step
  3. Do not claim comparable full-suite or baseline clearance before that
  Step 3 proof is accepted.

## Resumption Update: Step 3 provenance complete; separate 834 blocker active

- Last accepted progress: Steps 1 and 2 are complete. Step 3 — **Classify the
  rejected comparable full-suite gate** is complete as a read-only provenance
  decision. The comparable gate remains rejected: accepted
  `test_baseline.log` at `8418036b` is 3038/3038; the successful fresh-build
  full-suite `test_after.log` is 2520 passed / 518 failed, with 516 new
  failures and pass delta -518. No baseline replacement or clearance occurred.
- Interrupted step: Step 4 — **Obtain comparable baseline proof and return
  830**. It must not run until the separately scoped blocker is accepted.
- Blocker boundary: all 516 new failures first emit `LIR-owned aggregate
  function type requires a matching module owner` at
  `lir_owned_type_spec` (`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:106-108`),
  blamed to `b556c6c9f`. The inventory is 297 `llvm_gcc_c_torture`, 145
  `cpp_positive`, 66 `c_testsuite`, 3 `c_positive`, 2 `eastl_external`, 2
  `clang`, and 1 `abi`. This native aggregate key/module-tag lookup contract
  is outside 831's bounded completed 832/833 successor contracts.
- Boundary confirmation: `frontend_hir_tests` passes, and no failed output
  uses `truthiness_lhs_parameter_authority`; therefore this does not reopen
  closed 832 or closed 833.
- Active successor: `ideas/open/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
  owns only diagnosis and repair of the `lir_owned_type_spec` module-owner
  canonicalization/provenance relation, with representative multi-suite
  coverage. It does not perform baseline acceptance.
- Exact return point: after 834 has accepted its bounded repair and focused
  proof, reactivate 831 at Step 4 — **Obtain comparable baseline proof and
  return 830**. Run 831's supervisor-owned comparable full-suite gate; only an
  accepted gate may return 830 unchanged at Step 3. Do not return directly to
  830 or to any earlier 831 step.

## Resumption Update: closed 834; Step 4 comparable-baseline gate active

- 834 is capability-complete and archived at
  `ideas/closed/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`.
  Its Step 1 diagnosis and accepted bounded repair/proof restore the native
  aggregate key-to-module-owner relation without reopening 832/833.
- The only implementation attributed to the 834/835 return route is
  `932c3339b` (`hir: retain canonical aggregate owner identity`). 834's exact
  focused proof is 4/4; matching `test_before.log` and `test_after.log` are
  both 4/4, and the regression guard passed with allow-non-decreasing.
- Exact resumed return point: Step 4 — **Obtain comparable baseline proof and
  return 830**. Run the unchanged supervisor-owned full-suite/comparable
  baseline gate against the accepted 3038/3038 baseline. Only accepted
  comparable evidence may reactivate 830 at unchanged Step 3; do not infer
  baseline clearance from 834's focused proof.

## Resumption Update: Step 4 rejected; remaining LIR owner-rejection blocker active

- Step 4 comparable gate is rejected: accepted `test_baseline.log` is
  3038/3038; fresh `test_baseline.new.log` is 3026/3038, with 12 failed tests.
  The regression guard reports 10 new classified failures and a decreased pass
  count. No baseline replacement, clearance, or return to 830 occurred.
- Boundary evidence: the candidate contains remaining LIR aggregate-owner
  failures at both `LIR-owned aggregate function type requires a structured
  owner key` and `... requires a matching module owner`, plus the
  `frontend_lir_global_type_ref` no-owner compatibility assertion. This is not
  evidence that closed 832/833 regressions reopened, and it exceeds closed
  834's accepted focused relation.
- Active successor: `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
  owns first-owner decomposition of these remaining structured-key,
  matching-owner, and no-owner-compatibility contracts and only the smallest
  separately scoped repair route evidence that follows.
- Exact return point: after 836 accepts its bounded route and focused proof,
  reactivate 831 at Step 4 and rerun the unchanged comparable full-suite gate.
  Do not return directly to 830 or claim baseline clearance before that gate is
  accepted.
