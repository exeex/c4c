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
