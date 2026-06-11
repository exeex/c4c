# 193 Route 3 prepared-policy boundary hardening

## Goal

Harden the Route 3 memory/source boundary so route-first semantic reads cannot
replace prepared fallback, AArch64 target-addressing policy, or materialization
policy where those concerns still belong outside BIR route facts.

## Why This Exists

Idea 190 proved that a Route 3 semantic memory/source view can be locally
correct while broader AArch64 behavior regresses if prepared fallback and
target-policy boundaries are bypassed. The pre-Phase-E readiness audit makes
that lesson a blocker for retirement analysis rather than a one-off testcase
repair.

## In Scope

- Define explicit Route 3 route-first and prepared-fallback rules for memory
  source identity consumers.
- Separate target-neutral memory/source identity from target-owned addressing,
  frame, relocation, volatility, materialization, and final operand policy.
- Identify Route 3 consumers where route facts may be read first only when
  prepared policy remains authoritative for target-specific decisions.
- Add or specify proof coverage that catches fallback bypass and
  target-addressing regressions, including nearby same-feature cases.
- Document which prepared Route 3 helpers remain public migration oracles.

## Out Of Scope

- Moving AArch64 address-materialization or target-addressing policy into BIR
  route views.
- Deleting prepared memory/source helpers.
- Reworking unrelated routes under the Route 3 boundary hardening label.
- Treating the idea 190 regression as already sufficient proof for future
  route-first memory migrations.
- Opening draft 155 or contracting `PreparedBirModule` fields.

## Acceptance Criteria

- Route 3 has a durable boundary statement separating semantic source identity
  from prepared/target policy.
- Route-first consumers have explicit fallback rules for absent, mismatched,
  ambiguous, or policy-sensitive facts.
- Proof covers both a positive route-first case and a negative or fallback case
  that would fail if prepared target policy were bypassed.
- Public prepared Route 3 oracle/fallback surfaces are named and retained
  unless replacement coverage exists.
- Future Phase E retirement analysis can cite this boundary without treating
  Route 3 facts as authority over final target memory operands.

## Reviewer Reject Signals

- Describing idea 190 only as a fixed regression or narrow testcase.
- Moving address-materialization, frame, relocation, volatility, or final memory
  operand decisions into BIR route facts.
- Replacing prepared fallback with route-first reads without negative fallback
  tests.
- Claiming capability progress through expectation rewrites or by weakening an
  unsupported/supported-path contract.
- Proving only the previously failing case while leaving nearby memory/source
  policy cases unexamined.
- Hiding the old prepared-policy failure mode behind a renamed helper or facade.
