# Stack-Slot Pointer Select-Edge Dependency Materialization

Status: Open
Type: Prepared move-bundle dependency materialization idea
Parent: `ideas/closed/452_select_edge_source_producer_rematerialization.md`
Source Evidence: `build/agent_state/452_step4_residual_disposition/`
Owning Layer: RV64 select-edge rematerialization dependencies with stack-slot
pointer homes

## Goal

Define whether and how a select-edge out-of-SSA move may materialize a
source-producer dependency whose operand is a pointer value with a stack-slot
home, without weakening the existing register/immediate select-edge
rematerialization contract.

## Why This Exists

Idea 452 closed the register/immediate select-edge rematerialization route.
Fresh `20010329-1` evidence still fails at `unsupported_move_bundle_target_shape`
because the first edge `%t18 -> %t22` depends on `%t18 = bir.ule ptr %t15,
%t17`, where `%t17 = bir.inttoptr i32 %t16 to ptr` has stack-slot pointer home
`slot_id=2 offset=16`. That shape is not general branch operand materialization:
it is a dependency operand inside select-edge source-producer rematerialization.

## In Scope

- Audit the `%t18/%t17/%t22` select-edge failure in `20010329-1` and nearby
  prepared homes, publications, object-home facts, stack slot metadata, and
  source-producer dependency records.
- Define the authority required to materialize a stack-slot pointer dependency
  at a predecessor-edge move site, including slot identity, width, load policy,
  freshness, clobber safety, edge placement, pointer type/encoding, and target
  register constraints.
- Decide whether the `%t17` `inttoptr` stack-slot home can be consumed from
  prepared facts, must be rematerialized from its immediate source, or must
  remain fail-closed behind a producer/home contract.
- Add focused coverage for accepted stack-slot pointer dependency
  materialization and fail-closed missing, incoherent, stale, ambiguous, or
  non-pointer-compatible facts.
- Preserve the existing register/immediate select-edge rematerialization
  behavior and tests from idea 452.

## Out Of Scope

- Direct select-result branch publication, closed by idea 450.
- Register/immediate select-edge source-producer rematerialization, closed by
  idea 452.
- General stack-home branch operand or condition materialization for branch
  consumers, tracked by
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Pointer-value memory provenance publication, local/global store publication,
  and generic instruction-side lowering.
- Raw-BIR reconstruction, testcase-shaped matching, filename/function/block
  matching, expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- The `%t18/%t17/%t22` representative row is classified by accepted prepared
  authority versus exact missing producer, home, freshness, load, placement, or
  target constraints.
- Accepted materialization, if any, consumes explicit prepared facts and does
  not copy the successor/join-block `%t18` result on the predecessor edge.
- Missing, stale, ambiguous, unsupported, or policy-ambiguous stack-slot
  pointer dependencies remain fail-closed with focused coverage.
- Any broader stack-home branch consumer, pointer-provenance, instruction, or
  storage residual is routed separately instead of being folded into this idea.

## Reviewer Reject Signals

- Reject copying `%t18` from its successor/join-block register home on the
  predecessor edge.
- Reject loading or rematerializing `%t17` from a stack slot without explicit
  prepared authority for slot identity, width, freshness, clobber safety, and
  edge placement.
- Reject testcase-shaped fixes keyed to `20010329-1`, `%t17`, `%t18`, `%t22`,
  one stack slot, or one block name.
- Reject broad stack-home branch operand work, pointer-provenance work, or
  generic instruction lowering presented as this select-edge dependency
  packet.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
