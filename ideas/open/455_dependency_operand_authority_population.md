# Dependency-Operand Authority Population

Status: Open
Type: Prepared producer/population idea
Parent: `ideas/closed/454_edge_dependency_operand_materialization_authority.md`
Source Evidence: `build/agent_state/454_step4_residual_disposition/`
Owning Layer: Prepared output population and printing for dependency-operand
authority records

## Goal

Populate and expose concrete prepared dependency-operand authority records for
representative edge source-producer operands, starting with the `20010329-1`
`%t17` dependency, so later RV64 consumer work can rely on explicit prepared
authority instead of raw homes, casts, or object metadata.

## Why This Exists

Idea 454 added the shared metadata planner and predicate for dependency
operands, but Step 4 did not prove dump-visible populated records for the
representative `%t17` edge. Before returning to stack-slot pointer select-edge
consumer work, the producer/prepared pipeline needs to decide whether `%t17`
gets an explicit `rematerialize_cast_from_source`, `load_from_stack_slot`, or
fail-closed record and print enough evidence for review.

## In Scope

- Audit where dependency-operand authority inputs can be derived for
  `20010329-1` `%t17` and adjacent edge source-producer operands.
- Populate prepared dependency-operand authority records using the idea 454
  metadata surface.
- Print or otherwise expose representative records in prepared output so
  lifecycle review can verify concrete policy, status, edge identity, operand
  identity, value home, object linkage, cast/source identity, freshness, and
  clobber-safety.
- Add focused producer/prepared coverage for populated accepted records and
  fail-closed missing or incoherent population.
- Keep any RV64 consumer work blocked until populated explicit authority is
  present and tested.

## Out Of Scope

- RV64 target lowering for stack-slot pointer select-edge materialization.
- Copying successor/join-block compare results on predecessor edges.
- Defining the metadata types already closed by idea 454 except for minimal
  corrections needed to populate them.
- General stack-home branch operand/condition materialization, tracked by
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Pointer-value memory provenance publication, local/global store publication,
  and generic instruction-side lowering.
- Inferring authority from raw BIR shape, raw `inttoptr`, stack-slot spelling,
  object metadata alone, filenames, function names, block names, or one
  prepared dump.
- Expectation rewrites, unsupported-marker downgrades, allowlists, pass/fail
  accounting changes, or `test_baseline.new.log` churn.

## Acceptance Criteria

- Prepared output or focused producer/prepared tests show concrete
  dependency-operand authority records for the selected representative class.
- `%t17` is classified with explicit populated policy and status, or with an
  exact fail-closed reason explaining which producer fact remains absent.
- Positive and fail-closed tests cover population for cast-rematerialization
  and stack-load policy boundaries where expressible.
- No RV64 consumer packet is selected until populated authority is available
  and lifecycle review explicitly routes back to consumer work.

## Reviewer Reject Signals

- Reject RV64 object-emission changes that consume dependency-operand authority
  before population/printing is proven.
- Reject treating idea 454 metadata type existence as evidence that `%t17`
  has a populated authority record.
- Reject deriving `load_from_stack_slot` or `rematerialize_cast_from_source`
  from raw stack homes, raw `inttoptr`, object metadata alone, or testcase
  shape.
- Reject testcase-shaped fixes keyed to `20010329-1`, `%t17`, `%t18`, `%t22`,
  one stack slot, or one block name.
- Reject broad stack-home branch operand, pointer-provenance, or generic
  instruction lowering work presented as authority population progress.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
