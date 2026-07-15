# LIR PHI Incoming Producer Authority Repair

Status: Open
Type: bounded PHI producer-handoff baseline blocker
Blocked Parent: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Restore native current-function `LirValueId` authority for the failing PHI
incoming producer handoff exposed by the full baseline, without reopening the
accepted CFG/PHI schema or verifier contract.

## Why This Exists

At clean HEAD, the mandatory full baseline for 754 stopped at 1447/3037 on
`llvm_gcc_c_torture_src_vrp_2_c` with
`LirPhiIncoming.value: must identify a known current-function LirValueId`.
The clean build succeeded. `git blame` locates the relevant verifier
enforcement in `6ece9fe8f` (`Publish typed LIR PHI incoming authority`), but
this idea does not assume that commit is the root cause.

## In Scope

- Trace the single failing PHI incoming producer-to-consumer handoff and state
  the exact missing, stale, or foreign native authority fact.
- Make the smallest producer-side or immediate lowering-handoff repair that
  supplies a valid current-function `LirValueId` to the existing PHI contract.
- Add nearby same-family positive and malformed-authority coverage, then prove
  the bounded route before returning control to 754's full-baseline gate.

## Out Of Scope

- Reopening or weakening accepted CFG/PHI schema, edge, predecessor, or
  verifier authority.
- Aggregate/vector `LirExtractValueOp` work, Raw-BIR, pointer/object/memory,
  generic expression provenance, rendered-text recovery, or a broad residual
  instruction/terminator sweep.

## Acceptance Criteria

- The traced PHI incoming handoff supplies checked native current-function
  value authority to the existing contract.
- Missing, unknown, foreign, and stale authority remains rejected.
- Focused same-feature proof passes, and the supervisor can resume 754's
  required 100% full-baseline gate without changing its scope.

## Reviewer Reject Signals

- Reject any change that weakens `LirPhiIncoming.value` verification or permits
  raw/display text as identity merely to make `vrp_2.c` pass.
- Reject reopening accepted CFG/PHI predecessor or edge semantics, generic
  instruction/terminator conversions, or a catch-all provenance rewrite.
- Reject testcase-shaped special cases, expectation downgrades, or a green
  named test without nearby malformed-authority coverage.
- Reject claiming `6ece9fe8f` is causal without producer-handoff evidence.
