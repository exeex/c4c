# LIR Next Function-Body Parameter Authority Handoff

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md` post-Step 7.37
Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Select and publish one next valid function-body parameter-use semantic row as
a native structured LIR authority contract, so 734 can later receive exactly
that row without presentation-derived recovery.

## Why This Exists

734 accepted only DirectScalar parameter uses in a direct pointer GEP, binary
LHS, binary RHS, and exact return-value role. Its no-omission source contract
still includes other function-body parameter forms, but no additional row is
currently authorized for Raw-BIR receipt. The producer/schema/verifier first
owner must identify one native row before a receiver can proceed.

## In Scope

- Trace the next candidate function-body parameter-use route and select one
  semantic row only when its value identity, current-function ownership,
  parameter index, type, ABI classification, role, and exact consuming
  operand relation have native structured representation.
- Publish and verifier-check only that selected row, including malformed,
  missing, duplicate, foreign, owner/index/type/ABI/role, and
  consumer-incoherent rejection as applicable.
- Add focused same-feature producer positive/negative coverage and write an
  exact one-row receiver handoff for 734.

## Out Of Scope

- Raw-BIR containers, importer or receiver/verifier edits, broad parameter
  admission, declaration facts treated as body-use authority, ABI conversion,
  or any second parameter row.
- Reopening accepted direct-pointer, DirectScalar binary-LHS, binary-RHS, or
  ReturnValue rows; memory/VA, aggregate/vector, module/type/global/metadata,
  residual instruction/terminator, inline-assembly, target lowering, or text
  parsing.

## Acceptance Criteria

- Exactly one selected body-parameter semantic row has a native checked
  structured contract and focused nearby positive/negative proof.
- The handoff identifies its exact tuple, consumer relation, failure cases,
  and the one bounded 734 receiver return action; all nonselected forms stay
  fail closed.
- No Raw-BIR/importer code changes, presentation-derived authority, or
  expectation weakening is used to claim the handoff.

## Step 1 Selection Record

The sole selected row is an unchanged current-function integer parameter with
`LirNativeBodyParameterAbi::DirectScalar`, consumed directly as
`LirSwitch.selector`. Its required native tuple is the parameter-definition
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
`LirTypeRef`, `DirectScalar` ABI, a new `SwitchSelector` role, and the exact
consumer relation that `LirSwitch.selector` equals that value and
`LirSwitch.selector_type_ref` equals that type.

The first producer is `StmtEmitter::emit_control_flow_stmt(const SwitchStmt&)`;
the publication/verifier seam is a dedicated optional `LirSwitch` selector
authority checked by `verify_switch_selector`. It must not reuse the accepted
`LirBinOp.scalar_lhs_parameter_authority` or its materializing add: that is a
binary-LHS relation, not this direct switch-selector row. DirectPointer,
DirectScalar binary-LHS, binary-RHS, ReturnValue, and every other parameter
form remain fail closed.

## Resumption Record: Preserved lowering-selector worktree collision

Status: resumed after Idea 826 completed its isolation-only prerequisite.
Idea 825 does not own, accept, remove, or edit the preserved dirty 822
material.

- Last accepted progress: Step 1 only, accepted in `7e6366cc9` (`plan: select
  direct scalar switch selector authority row`). No Step 2 implementation,
  proof, or acceptance commit exists.
- Completed runbook steps: Step 1 selected the unchanged current-function
  `DirectScalar` integer parameter consumed directly as `LirSwitch.selector`.
- Interrupted step: Step 2, *Publish and verify the selected authority tuple*.
- Blocker and scope boundary: the preserved dirty `stmt.cpp` change documented
  by Idea 822 detects the same direct-scalar switch selector but materializes
  an `add` carrying `LirBinOp.scalar_lhs_parameter_authority`. That is the
  binary-LHS/materializing route Step 2 expressly rejects, and editing around
  it would overwrite or blend an unaccepted Idea 822 slice. Its companion
  dirty test patch and Idea 821 note remain preserved and unaccepted.
- Isolation resolution and exact return action: Idea 826 closed its bounded
  isolation route with the dirty 822 material recoverably preserved at
  `.git/c4c-preservation/822-materializing-add-and-lowered-route.patch`.
  Restore it only with `git apply --binary .git/c4c-preservation/822-materializing-add-and-lowered-route.patch`.
  Resume this runbook exactly at Step 2 and publish a dedicated optional
  `LirSwitch` selector authority with the `SwitchSelector` role directly from
  the native parameter definition; do not reuse
  `scalar_lhs_parameter_authority` or materialize an add. Then define the
  focused producer proof before proceeding to Step 3.
- Proof and commit status: `7e6366cc9` is the only accepted Idea 825 progress.
  There is no Step 2 proof. The completed isolation prerequisite must not be
  mistaken for authority acceptance or performed again by Idea 825.

## Resumption Record: native DirectScalar binary-LHS authority blocker

Status: deactivated for separately scoped Idea 827; resume only after its
accepted binary-LHS producer/verifier repair.

- Last accepted progress and completed steps: Step 1 only, accepted in
  `7e6366cc9` (`plan: select direct scalar switch selector authority row`).
  Step 2 has no accepted semantic progress.
- Interrupted step: Step 2, *Publish and verify the selected authority tuple*.
  The current shared-worktree `core.cpp`, `ir.hpp`, `verify.cpp`, and focused
  test changes are preserved, unaccepted implementation only; do not claim,
  commit, discard, or alter them during the blocker.
- Build and focused evidence: fresh `cmake --build --preset default` passed.
  The matching exact `^frontend_lir_call_type_ref$` CTest was 0/1 before at
  the earlier `LirSwitch.selector` abort and remains 0/1 after, now at
  `LirBinOp.scalar_lhs_parameter_authority: is required when LirBinOp.lhs uses
  a native direct-scalar parameter`.
- First bad fact and scope boundary: the newly exposed native DirectScalar
  `LirBinOp` LHS producer/verifier gap is outside this source's selected direct
  `LirSwitch.selector` row. It belongs solely to Idea 827; closed 820's prior
  `ull` publication does not authorize it.
- Exact return point and commit status: after Idea 827 receives accepted proof,
  resume exactly at Step 2. Reassess the preserved Step 2 selector slice using
  its own proof; there is no Idea 825 Step 2 acceptance commit.

## Return From Idea 827

- Idea 827 is intentionally concluded: its exact first failing operation was
  unary `fneg`, and no truthful binary definition/operation type-mismatch
  reproduction exists. Idea 828 accepted the unary route in `524b24f64`.
- Resume exactly at Step 2, *Publish and verify the selected authority tuple*.
  The current dirty selector-authority implementation remains unaccepted and
  must receive its own focused proof; do not credit 827/828 evidence.

## Reviewer Reject Signals

- Reject selection from rendered parameter names, types, signatures, operand
  spelling, printer output, or testcase shape rather than native authority.
- Reject generic parameter admission, broad ABI conversion, multiple rows,
  receiver edits, expectation downgrades, or verifier weakening claimed as
  this one-row handoff.
- Reject retaining a missing/ambiguous/foreign/consumer-incoherent authority
  path behind a renamed carrier or leaving selected malformed input accepted.
- Reject any reopening of the four accepted 734 parameter receiver rows or
  expansion into non-parameter semantic families.
