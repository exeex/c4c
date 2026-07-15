# LIR PHI Residual Producer-Family Authority Trace

Status: Closed
Type: bounded PHI residual-producer baseline blocker
Blocked Parent: `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`

## Goal

Trace and repair, only if the trace supports it, the native current-function
`LirValueId` handoff missing from the four residual PHI incoming failures that
remain after 804's accepted scalar unary-minus repair.

## Why This Exists

After 805 restored a coherent checkout, the required full baseline built but
passed only 3033/3037. `ieee/pr50310.c`, `20000715-1.c`, `20060910-1.c`, and
`pr68376-2.c` each fail the existing `LirPhiIncoming.value` current-function
ID check. This diagnostic alone does not establish the selected scalar
unary-minus seam from 804: its focused `vrp_2.c` proof remains passing.

## In Scope

- Reproduce and trace each named residual case from PHI incoming to its
  immediate producer/lowering handoff; group cases only when their native
  producer path is demonstrably the same.
- Select one evidenced producer family or immediate handoff at a time and make
  the smallest native-authority repair required by the existing PHI contract.
- Add nearby same-family positive and malformed-authority coverage and obtain
  focused proof before the supervisor reruns the required full baseline.

## Out Of Scope

- Reopening 804's scalar unary-minus repair, CFG/PHI schema, verifier,
  predecessor, or edge semantics.
- A broad residual instruction/terminator sweep, generic provenance rewrite,
  Raw-BIR, aggregate/vector, pointer/object, text recovery, or testcase-name
  branching.
- Declaring 804 or 754 clear from a partial baseline.

## Acceptance Criteria

- Each named failure is either linked to an evidenced native producer family
  or explicitly classified into a separately scoped successor before any
  repair expands beyond that family.
- Any selected repair supplies a checked current-function `LirValueId` through
  the existing PHI incoming contract; missing, foreign, stale, and unknown
  authority still rejects.
- Focused same-feature coverage passes, and the supervisor accepts a fresh
  100% full baseline before returning 804 at unchanged Step 3.

## Residual Successors

806 retains only the shared postfix-increment old-value handoff selected by
Step 1. The unshared producer families are separately scoped and must not be
absorbed into its Step 2 repair:

- Closed `ideas/closed/807_lir_phi_floating_unary_minus_fneg_authority.md`
  resolved `ieee/pr50310.c` and floating unary-minus `fneg` authority.
- Closed `ideas/closed/808_lir_phi_scalar_bit_not_xor_authority.md` resolved
  `pr68376-2.c` and scalar bit-not `xor` authority.

## Reviewer Reject Signals

- Reject treating an identical `LirPhiIncoming.value` diagnostic as proof that
  the four cases share 804's unary-minus producer handoff.
- Reject weakening PHI verification, deriving identity from `%t` text,
  rendered output, instruction order, or testcase names.
- Reject a catch-all instruction/terminator conversion, generic provenance
  rewrite, or changes outside the traced producer family presented as progress.
- Reject named-case-only bypasses, expectation downgrades, or accepting any
  below-100% full baseline as parent clearance.

## Parent Return Contract

804 Steps 1–2 remain accepted in `308fff39c`. This blocker starts by tracing
the four residual tests; it must not repeat or alter the scalar unary-minus
route. Once accepted proof includes a supervisor-accepted 100% full baseline,
reactivate 804 at Step 3, *Prove the blocker and return it to 754*.

## Resumption Record

- Last accepted progress: Steps 1–2 are accepted. Step 1 separated the
  residual families; Step 2 repaired only the postfix-increment old-value
  producer handoff.
- Interrupted runbook step: Step 3, *Prove the blocker and return to 804*.
- Resolved blockers and scope boundary: 807 closed floating unary-minus
  `fneg` authority in `8f31e2535`; 808 closed scalar bit-not `xor` authority
  in `b86df3b9d`. Both producer families were separately scoped outside 806's
  selected postfix route.
- Exact return point: resume at Step 3 full-baseline proof only after 807 and
  808 have accepted their focused routes and the follow-on full baseline can
  be evaluated.
- Remaining next action: resume Step 3 by obtaining and having the supervisor
  accept the required 100% full baseline, then reactivate 804 at its unchanged
  Step 3.
- Accepted implementation and proof: commit `961ce9fda` accepted the Step 2
  postfix authority repair. Focused proof passed via
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`,
  with the matching non-decreasing frontend-HIR before/after guard and
  `build/tests/frontend/frontend_lir_call_type_ref_test` also passing.
- Successor closure evidence: 808's fresh focused
  `frontend_lir_call_type_ref` proof passed 1/1 after a fresh build. Its
  strict guard rejected unchanged 1/1 -> 1/1 counts, then the matching
  non-decreasing guard mode `--allow-non-decreasing-passed` exited 0 and was
  supervisor-accepted. This is focused successor acceptance, not parent
  clearance.

## Resumption Update — switched to scalar dereference-load authority blocker

- Last accepted progress and completed steps: Step 1 classified the residual
  producer families, and Step 2's postfix-increment old-value repair was
  accepted in `961ce9fda` with the focused proof recorded above. Step 3's
  trace is complete but did not authorize a 806 repair.
- Interrupted step: Step 4, *Repair the classified in-scope producer handoff
  and prove the blocker*.
- Blocker and scope boundary: `20060910-1.c`'s `check_header` ternary true
  expression, `*((deeper)->buffer_position)++`, preserves existing incoming
  authority through conditional lowering. The enclosing scalar
  `UnaryOp::Deref` emits a `LirLoadOp` through a fresh temporary string and
  loses the immediate `LirValueId` into the conditional PHI. That
  dereference-load handoff is not the accepted postfix producer route and is
  therefore owned by separately scoped
  `ideas/closed/809_lir_phi_scalar_dereference_load_authority.md`.
- Exact return point: after 809 has an accepted repair and the supervisor has
  accepted a fresh 100% full baseline, resume 806 at Step 3, *Prove the
  blocker and return to 804*, without repeating Steps 1–2.
- Remaining action: evaluate that full-baseline result at 806 Step 3; only a
  supervisor-accepted 100% result may reactivate 804 at its unchanged Step 3.
- Accepted proof references: `961ce9fda` accepted the postfix repair with the
  recorded fresh build, matching frontend-HIR guard, and
  `frontend_lir_call_type_ref_test`; `8f31e2535` and `b86df3b9d` remain the
  separately closed `fneg` and scalar bit-not `xor` routes.

## Closure Record

Capability complete. Steps 1--2 remain accepted, including postfix repair
`961ce9fda`; 807 and 808 remain the separately closed `fneg` and `xor`
routes; and 809 completed the separately scoped scalar dereference-load
handoff in `4d29f7b3e`. The supervisor accepted its fresh full baseline at
3037/3037 after `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` (build had no work; total about 29.11s). This is the
required Step 3 full-baseline result, so no intermediate 806 runbook step or
duplicate proof is required. Return directly to 804 at unchanged Step 3.
