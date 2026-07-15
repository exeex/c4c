# Next LIR Body-Parameter Authority Handoff Runbook

Status: Active
Source Idea: ideas/open/825_lir_next_body_parameter_authority_handoff.md
Resumed from: Idea 826 isolation-only closure at Step 2
Supersedes: 734 Step 7.37 receiver runbook after source-gate rejection

## Purpose

Establish one and only one next native function-body parameter authority row
before 734 resumes its Raw-BIR receiver route.

## Core Rule

Choose a row only from native structured LIR facts. Do not derive semantic
authority from names, signatures, rendered operands, printer output, or tests.
No Raw-BIR/importer work belongs in this plan.

## Read First

- `ideas/open/825_lir_next_body_parameter_authority_handoff.md`
- `ideas/closed/826_shared_worktree_switch_authority_slice_isolation.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` (post-Step 7.37
  resumption record)
- closed 824's return-value handoff only as accepted non-repeat context

## Non-Goals

- Generic parameter admission, ABI conversion, a second parameter row, or
  receipt into Raw-BIR.
- Repeating 734's accepted pointer, DirectScalar LHS/RHS, or ReturnValue rows.
- Any non-parameter family or presentation-based recovery.

## Ordered Steps

### Step 1 - Trace and select one native body-parameter use row (completed)

Goal: identify one next receiver-eligible parameter-use semantic relation and
its first owning producer/schema/verifier seam.

Actions:

- inspect existing native body-use production and verifier paths;
- select exactly one row only if the full authority tuple and consuming
  relation can be represented natively;
- otherwise record the exact missing first-owner fact and repair this plan
  before any implementation.

Completion check: selected `DirectScalar` integer current-function parameter
used directly as `LirSwitch.selector`; its tuple is parameter `LirValueId`,
`LirFunction.link_name_id` owner, parameter index, `LirTypeRef`,
`LirNativeBodyParameterAbi::DirectScalar`, new `SwitchSelector` role, and the
exact `LirSwitch.selector`/`selector_type_ref` relation. The native seam is
`StmtEmitter::emit_control_flow_stmt(const SwitchStmt&)`, `LirSwitch`, and
`verify_switch_selector`. Existing binary-LHS/RHS, ReturnValue, DirectPointer,
and every other parameter form remain fail closed; no Raw-BIR changes.

### Step 2 - Publish and verify the selected authority tuple

Goal: add only the selected producer/schema/verifier carrier and malformed
authority rejection. Publish a dedicated optional `LirSwitch` selector
authority with a `SwitchSelector` role; do not route through or reuse
`LirBinOp.scalar_lhs_parameter_authority` or its materializing add, because
that is the accepted binary-LHS row rather than the selected direct consumer
relation.

Completion check: the selected tuple and consuming relation verify natively;
missing, foreign, duplicate, owner/index/type/ABI/role-invalid, selector- or
selector-type-incoherent, and display-derived paths fail closed.

### Step 3 - Prove and hand off the selected row to 734

Goal: establish focused nearby producer proof and a precise one-row receiver
handoff.

Completion check: focused positive/negative proof is accepted and the source
idea records the exact 734 return action without crediting Raw-BIR receipt.
