# Edge Dependency-Operand Materialization Authority

Status: Open
Type: Prepared producer/home metadata authority idea
Parent: `ideas/closed/453_stack_slot_pointer_select_edge_dependency_materialization.md`
Source Evidence: `build/agent_state/453_step4_residual_disposition/`
Owning Layer: Prepared dependency-operand materialization policy for edge
source producers

## Goal

Add a durable prepared authority surface for dependency operands used by
edge source-producer rematerialization, so later RV64 consumers can decide
whether an operand may be loaded from a stack slot, rematerialized from a cast
source, or must remain fail-closed.

## Why This Exists

Idea 453 classified the `20010329-1` `%t18/%t17/%t22` select-edge dependency
and found no sound implementation packet. Current prepared facts show `%t17`
has a stack-slot pointer home and `%t16` is a rematerializable immediate, but
they do not state a dependency-operand policy for `%t17`. Target lowering must
not infer `load_from_stack_slot` or `rematerialize_cast_from_source` from raw
`inttoptr`, stack-slot spelling, or object metadata alone.

## In Scope

- Audit prepared producer/home facts for edge dependency operands, starting
  with `%t17` in `20010329-1`.
- Define an explicit dependency-operand materialization authority surface for
  edge source producers.
- Include a policy enum or equivalent fact for `load_from_stack_slot`,
  `rematerialize_cast_from_source`, and fail-closed/unsupported states.
- Tie each dependency operand to value-home and object metadata, including
  slot identity, width, freshness, clobber-safety, edge placement, value type,
  cast/source identity, and target compatibility.
- Add focused producer/prepared tests for positive authority and fail-closed
  missing, stale, ambiguous, incoherent, non-pointer-compatible, or
  unsupported facts.

## Out Of Scope

- RV64 consumer lowering for stack-slot pointer select-edge materialization.
- Copying successor/join-block source results on predecessor edges.
- General stack-home branch operand or condition materialization, tracked by
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Pointer-value memory provenance publication, local/global store publication,
  and generic instruction-side lowering.
- Inferring policy from raw BIR shape, `inttoptr`, stack-slot spelling, object
  metadata alone, filenames, function names, block names, or one prepared dump.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- Prepared output exposes an explicit dependency-operand materialization
  authority record or equivalent fact for the selected edge dependency class.
- `%t17`-style stack-slot pointer dependencies are classified by accepted
  `load_from_stack_slot`, accepted `rematerialize_cast_from_source`, or exact
  fail-closed reason.
- Focused tests cover positive producer/prepared facts and fail-closed missing
  policy, stale/clobbered slot, ambiguous home/object linkage, missing cast
  source, unsupported type, and missing edge placement.
- No RV64 target consumer packet is selected until producer-owned authority is
  present and tested.

## Reviewer Reject Signals

- Reject target-lowering changes that consume `%t17` before producer/prepared
  dependency-operand authority exists.
- Reject treating a stack home plus object metadata as sufficient
  `load_from_stack_slot` authority without freshness and clobber-safety.
- Reject treating raw `inttoptr` plus a rematerializable integer source as
  sufficient `rematerialize_cast_from_source` authority without an explicit
  dependency record.
- Reject testcase-shaped fixes keyed to `20010329-1`, `%t17`, `%t18`, `%t22`,
  one stack slot, or one block name.
- Reject broad stack-home branch operand, pointer-provenance, or generic
  instruction lowering work presented as dependency-operand authority progress.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
