# Direct-Call Structured Argument Identity Prerequisite Runbook

Status: Active
Source Idea: ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md
Activated from: 829 Step 2 separate-blocker decision

## Purpose

Create only the missing native LIR structured argument-1 identity/type
producer relation required before 829 can assess authority publication.

## Core Rule

Use native structured construction and verifier facts only. Do not recover
identity or type from text, names, signatures, rendered operands, diagnostics,
or parser-shaped representations.

## Non-Goals

- Any body-parameter authority publication, role, or Raw-BIR receiver work.
- Generic call argument handling, other call forms/indices, ABI conversion,
  or unrelated 821/822 material.

## Ordered Steps

### Step 1 - Trace the smallest native structured producer seam

Goal: identify how the selected argument-1 call relation can be represented
with native identity and exact type at construction time.

Actions:

- inspect only the direct-call construction, native LIR representation, and
  verifier seam;
- demonstrate the current non-SSA/text-only gap without treating signature or
  display data as a substitute; and
- select one bounded direct/non-variadic/specified argument-1 relation.

Completion check: the intended structural producer/verifier contract is
explicit and no generic or presentation-derived route is admitted.

### Step 2 - Produce and verify the structural argument relation

Goal: implement the selected native identity/type carrier and fail-closed
producer verification.

Actions:

- populate the relation directly at the native LIR construction seam;
- verify presence, ownership/coherence, direct-call form, argument index, and
  exact callee parameter-1 type; and
- leave all other call forms and argument indices outside the contract.

Completion check: the relation is structural and verifier-checked without any
829 authority tuple or presentation recovery.

### Step 3 - Diagnose and repair the rejected post-commit baseline

Goal: resolve the rejected full-suite acceptance gate before 830 can continue
or produce another commit.

Actions:

- diagnose the post-`f0fc85e4f` candidate regression from 3038/3038 to
  3024/3038, including the `frontend_hir_tests` segfault and 13
  `llvm_gcc_c_torture` failures;
- determine whether the new failures are caused by the 830 slice and repair
  only an in-scope cause, without broadening the native relation contract; and
- rerun a comparable full-suite baseline and obtain supervisor acceptance that
  no new baseline problem remains before any further commit.

Completion check: the rejected candidate is replaced by accepted comparable
proof of no new baseline problem. Focused proof alone cannot clear this gate.

### Step 4 - Prove the prerequisite and return to 829

Goal: establish focused same-feature positive and malformed proof, then
document the exact parent return condition.

Actions:

- run a fresh build and focused producer proof selected by the supervisor;
- cover missing or incoherent structural relation rejection; and
- record the resulting relation and reactivate 829 only if it can support its
  existing parameter-definition tuple without recovery.

Completion check: after Step 3 clears the baseline gate, 830 can conclude as
a producer prerequisite and 829's exact Step 2 return action is durable.
