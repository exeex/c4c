# Remaining LIR Aggregate-Owner Rejection Decomposition Blocker

Status: Open
Type: bounded LIR aggregate-owner residual-family decomposition prerequisite
Blocked Parent: `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`, Step 4 comparable full-suite proof

## Goal

Identify the first owning contract for the remaining LIR aggregate-owner
rejections exposed by 831's rejected comparable gate, then repair only the
smallest demonstrated relation needed to remove that residual family without
weakening legitimate ownerless rejection.

## Why This Exists

831 Step 4's fresh candidate is 3026/3038 against the accepted 3038/3038
baseline. The failures include both `LIR-owned aggregate function type requires
a structured owner key` and `... requires a matching module owner`, as well as
the `frontend_lir_global_type_ref` no-owner compatibility assertion. Closed 834
accepted a focused durable-owner relation but did not establish these remaining
contracts across the full-suite paths. Treating them as 831 implementation or
as a 834 reopening would silently widen accepted work.

## In Scope

- Diagnose the first owner for the structured-key, matching-module-owner, and
  no-owner-compatibility residual groups from the comparable-gate evidence.
- Determine with direct evidence whether they share one native LIR relation or
  require ordered separately scoped successors.
- Implement only a demonstrated native relation and prove it with nearby
  coverage from more than one affected path while preserving invalid, foreign,
  wrong-namespace, and genuinely ownerless behavior.
- Return accepted bounded evidence to 831 Step 4 only.

## Out Of Scope

- 831's comparable-baseline decision, 830 direct-call work, 829 authority,
  Raw-BIR, generic-call work, test-harness changes, or baseline replacement.
- Reopening closed 832/833/834 without direct first-owner evidence.
- Test expectation changes, allowlists, filtering, rendered-text matching, tag
  fallbacks, or weaker no-owner diagnostics as a substitute for native repair.

## Acceptance Criteria

1. The initial packet records the rejected 3038-to-3026 comparable-gate delta
   and separates the three observed residual contract groups by first owner.
2. The repair route is either one evidenced native LIR relation or explicit
   ordered successors; no mixed implementation is authorized without a shared
   owning seam.
3. Any implemented route has fresh focused multi-path proof and preserves the
   malformed/foreign/wrong-namespace/ownerless contract.
4. Only after accepted bounded evidence may 831 resume its unchanged Step 4
   comparable gate; this idea makes no baseline-clearance or 830-return claim.

## Reviewer Reject Signals

- Reject merging structured-key, matching-owner, and no-owner compatibility
  failures solely because their diagnostics mention aggregate types.
- Reject testcase-specific branches, rendered-text probes, expectation
  downgrades, allowlists, filtering, or harness changes as repair evidence.
- Reject a tag fallback or accepting genuinely ownerless/foreign/wrong-
  namespace input to hide a valid-owner lookup failure.
- Reject reopening 832, 833, or 834 without direct first-owner evidence, or
  claiming full baseline clearance from focused proof.
- Reject absorbing 831/830 scope or adjacent aggregate metadata redesign.

## Resumption Record: user-priority switch to 837 architecture umbrella

Status: parked by lifecycle switch to
`ideas/open/837_lir_nominal_type_family_architecture.md`.

- Switch reason: the user explicitly prioritized the nominal type-family
  architecture umbrella before continuing this residual repair route. This is
  a priority switch, not evidence that 837 satisfies, supersedes, or closes
  836.
- Last accepted progress: none inside 836 beyond lifecycle activation and the
  incoming rejected-gate evidence. No 836 runbook step, implementation, or
  proof has been accepted.
- Interrupted step: Step 1, **Decompose the remaining aggregate-owner rejection
  families**.
- Durable carry-in evidence: parent 831 Step 4 compared accepted
  `test_baseline.log` at 3038/3038 with `test_baseline.new.log` at 3026/3038.
  The candidate had 12 failures; the regression guard reported 10 newly
  classified failures and a decreased pass count. No fresh 836 proof exists.
- Exact return point: resume 836 at unchanged Step 1. Reproduce current evidence
  first, then identify separate first owners for the structured-key,
  matching-module-owner, and no-owner compatibility groups before any code
  edit. Do not infer that the old 3026/3038 candidate still reproduces after
  the architecture initiative.
- Remaining work: Step 1 first-owner decomposition, Step 2 smallest evidenced
  native relation repair or ordered separately scoped successors, and Step 3
  focused multi-path proof and return decision.
- Parent return obligation: after 836 accepts its bounded route and proof,
  reactivate `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`
  at unchanged Step 4 for its comparable full-suite gate. Do not return
  directly to 830 or 829.
- Resume condition: after the 837 architecture initiative and its generated
  priority successor work, resume 836 only if fresh current evidence still
  reproduces an independently owned residual family. If the new architecture
  changes the facts, plan-owner must reclassify from current evidence rather
  than silently declaring 836 complete or superseded.
