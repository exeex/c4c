# LIR I686 Long-Width Policy Convergence Runbook

Status: Active
Source Idea: ideas/open/743_lir_i686_long_width_policy_convergence.md
Activated from: no active plan after retirement of the evidence-blocked idea-746 runbook

## Purpose

Converge the structured target-profile policy for `long` and `unsigned long`
across LIR production and verification and new-BIR receipt, preserving the C
32-bit I686 rule and the 64-bit LP64 rule.

## Goal

Establish one target-profile-aware width authority used consistently by the
producer, verifier, and receiver surfaces, with focused cross-target proof and
an exact idea-734 handoff.

## Core Rule

Use structured target-profile authority. Do not infer width from rendered IR,
target-triple text, names, test paths, or a target-specific matcher; do not
change I686 C `long` semantics merely to preserve the current unconditional
LIR mirror.

## Read First

- `ideas/open/743_lir_i686_long_width_policy_convergence.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- shared LLVM type production and LIR signature/verification surfaces
- new-BIR type lowering, signature/return/global receipt, and existing
  cross-target coverage

## Scope

- structured `long`/`unsigned long` width policy for supported target profiles
- LIR signature production and verification, new-BIR parameter/return/global
  receipt, and their focused positive and malformed proof
- an exact consumer handoff for only the idea-734 long parameter rows

## Non-Goals

- broad ABI, target-profile, pointer-width, long-double, aggregate, variadic,
  canonical-BIR, allocation, MIR, or emission redesign
- receiving unrelated idea-734 rows or treating this runbook as idea-734
  completion
- display-derived type selection, expectation weakening, allowlists, or
  target-specific testcase branches

## Execution Rules

1. Keep each code packet to one shared structured policy seam and its matching
   cross-target proof.
2. Preserve neighboring `int`, `long long`, floating, and pointer behavior.
3. Require fresh build, focused I686 and LP64 positive/negative proof, relevant
   backend proof, and the supervisor-selected regression checkpoint before the
   handoff.
4. Do not claim idea 734 unblocked beyond the exact accepted long rows.

## Ordered Steps

### Step 1 - Inventory the current width authorities and conflict

Goal: establish a checked matrix of every supported target-profile decision and
affected producer, verifier, and receiver surface before changing policy.

Actions:

- trace `long`/`unsigned long` width through shared LLVM type production, LIR
  signature mirrors and verification, and new-BIR parameter, return, and
  global lowering
- record the current I686/LP64 behavior, structured profile inputs, existing
  coverage, and the exact conflicting unconditional mirrors
- bind focused cross-target positive and malformed probes to the affected
  seams, keeping unrelated types as regression neighbors

Completion check:

- a checked matrix identifies one possible structured policy seam and every
  producer/verifier/receiver obligation needed to remove the conflict

### Step 2 - Publish the structured target-aware width policy

Goal: make the bounded producer, verifier, and receiver surfaces agree on the
same profile-aware `long`/`unsigned long` widths.

Actions:

- implement the smallest shared structured policy seam needed for I686 32-bit
  and LP64 64-bit behavior
- update LIR signature production and reachable malformed-mirror verification
  to use that authority
- update the corresponding new-BIR signature, return, and global receipt
  surfaces without widening scope to unrelated type families

Completion check:

- declarations and definitions publish and verify matching target-correct
  mirrors, and receiver surfaces accept only matching typed facts

### Step 3 - Prove cross-target behavior and hand off

Goal: prove the policy end-to-end and publish the exact bounded consumer
contract.

Actions:

- run fresh build, focused I686 and LP64 positive/negative coverage, relevant
  backend proof, and the supervisor-selected regression checkpoint
- verify transactional rejection for malformed or conflicting width mirrors
- record the exact structured facts and accepted idea-734 long parameter rows;
  leave every other blocked idea-734 row fail-closed

Completion check:

- accepted proof confirms I686 32-bit and LP64 64-bit policy across signatures,
  returns, and globals, with a precise bounded handoff to idea 734

## Handoff

This runbook may unblock only the `long`/`unsigned long` parameter rows named
by idea 743. It does not resolve the separate ordinary-value identity and
unresolved-external producer blockers that keep idea 734 inactive.
