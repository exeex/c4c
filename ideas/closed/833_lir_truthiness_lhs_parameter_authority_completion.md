# LIR Truthiness-LHS Parameter Authority Completion

Status: Closed — capability complete
Type: bounded native LIR truthiness-LHS producer/verifier authority completion
Blocked Parent: `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`, Step 2
Ordered After: `ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md`

## Goal

Supply and verify the missing native
`LirCmpOp.truthiness_lhs_parameter_authority` relation for the selected
direct-scalar truthiness LHS family, allowing the 13 identified GCC torture
tests to satisfy the existing verifier contract.

## Why This Exists

All 13 identified torture failures stop at unchanged
`verify_truthiness_lhs_parameter_authority` because the native authority is
missing for a direct-scalar truthiness LHS. This is distinct from the HIR
aggregate-owner crash and must run only after 832 returns accepted focused
proof to 831.

## In Scope

- Trace the selected direct-scalar truthiness LHS producer and existing
  verifier contract.
- Populate the minimum native authority relation and verify identity, owner,
  parameter, and type coherence at that seam.
- Add nearby positive and malformed/missing/foreign/incoherent authority
  coverage, then prove the 13 named torture cases progress through the former
  verifier rejection.

## Out Of Scope

- HIR aggregate-owner/function-parameter crash work, including 832's seam.
- Direct-call argument identity, 830/829 authority, Raw-BIR, generic compare
  or call rewrites, other truthiness forms, and ABI redesign.
- Text/signature/diagnostic recovery, default authority classification,
  test filtering, expectation weakening, unsupported markers, or harness
  changes.

## Acceptance Criteria

1. The selected direct-scalar truthiness LHS has an existing native authority
   relation produced and checked without presentation-derived recovery.
2. Missing, foreign, owner-incoherent, parameter-incoherent, or type-incoherent
   authority fails closed in nearby coverage.
3. A fresh build and focused proof show the 13 named torture cases no longer
   stop at `verify_truthiness_lhs_parameter_authority` for the missing
   relation.
4. The completion record returns to 831 Step 2 for proof collection and its
   later comparable full-suite gate; it does not claim baseline clearance.

## Reviewer Reject Signals

- Reject any HIR aggregate-owner, direct-call, 830/829, Raw-BIR, or generic
  compare/call rewrite presented as this native truthiness relation.
- Reject text-, signature-, name-, operand-, or diagnostic-derived authority,
  default classification, or a verifier exception for the named torture cases.
- Reject expectation downgrades, unsupported markers, allowlists, test
  filtering, or weaker harness contracts as proof of capability.
- Reject a broad all-truthiness or ABI redesign when the direct-scalar LHS
  producer/verifier seam can be repaired locally.

## Completion Record

Disposition: capability complete for the bounded direct-scalar
truthiness-LHS producer/verifier authority relation.

- Completed Steps 1, 2a, 2b, and 3. Step 1 specified the fail-closed native
  contract; Step 2a located the actual binary-`Ne` construction producer;
  Step 2b populated that native relation with generated-path and malformed
  verifier coverage; and Step 3 supplied accepted focused proof.
- Accepted implementation: `9b5046952`.
- Accepted proof: fresh build plus matching canonical exact 13-case logs,
  accepted by the supervisor regression guard, improved 0/13 before to 9/13
  after with zero new failures.
- Scope boundary: this does not claim comparable full-suite or baseline
  clearance. The residual `20090113-2`, `comp-goto-1`, `pr51323`, and
  `pr88714` failures remain aggregate-owner-family errors outside this idea.
- Successor return: resume
  `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
  at Step 2 to collect the accepted 832 and 833 evidence, then run its Step 3
  comparable full-suite gate before returning 830 unchanged at Step 3.
