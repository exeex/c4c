# RV64 Select-Edge Cast Dependency Consumer

Status: Open
Type: RV64 prepared consumer idea
Parent: `ideas/closed/455_dependency_operand_authority_population.md`
Source Evidence: `build/agent_state/455_step4_residual_disposition/`
Owning Layer: RV64 select-edge move materialization with explicit
dependency-operand cast-rematerialization authority

## Goal

Teach the RV64 select-edge move materializer to consume explicit populated
`rematerialize_cast_from_source` dependency-operand authority for edge
source-producer compares, starting with the `20010329-1` `%t17` dependency,
without copying successor/join-block compare results or inferring authority
from raw BIR shape.

## Why This Exists

Ideas 454 and 455 added and populated the dependency-operand authority needed
by the `20010329-1` `%t18 -> %t22` select-edge failure. Prepared output now
prints an available record for `%t17`:
`policy=rematerialize_cast_from_source status=available`, with `%t16` as a
rematerializable immediate cast source. That makes a bounded RV64 consumer
packet selectable. The stack-load route remains fail-closed as
`missing_stack_freshness`.

## In Scope

- Audit the populated `rematerialize_cast_from_source` record for the
  `20010329-1` `%t17` dependency and the existing select-edge compare
  rematerialization path.
- Define the RV64 consumer conditions for dependency-operand cast
  rematerialization: matching edge, source producer, operand role, dependency
  value, cast producer, cast source, source home, policy, status, and target
  register compatibility.
- Implement the smallest semantic consumer packet that rematerializes the cast
  dependency from explicit prepared authority before rematerializing the edge
  compare.
- Add focused object-emission coverage for accepted explicit cast dependency
  authority and fail-closed missing, unavailable, mismatched, stack-load-only,
  or successor-copy cases.
- Re-probe `20010329-1` and classify any remaining first owner after the
  consumer packet.

## Out Of Scope

- Producing dependency-operand authority records, closed by idea 455.
- Consuming `load_from_stack_slot` dependency policies; stack-load remains
  fail-closed until a freshness/clobber-safety producer idea exists.
- Copying `%t18` or any successor/join-block source result on a predecessor
  edge.
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

- RV64 consumes only populated `rematerialize_cast_from_source status=available`
  dependency-operand authority for the selected edge dependency shape.
- Missing, unavailable, mismatched, stack-load-only, or successor-copy cases
  remain fail-closed with focused tests.
- `20010329-1` no longer fails for the `%t17` cast dependency owner, or the
  next first owner is classified with evidence.
- No stack-load, pointer-provenance, generic instruction, or expectation-only
  change is claimed as cast dependency consumer progress.

## Reviewer Reject Signals

- Reject RV64 changes that infer cast dependency rematerialization from raw
  `inttoptr`, `%t16` immediates, stack homes, object metadata, or testcase
  shape instead of populated dependency-operand authority.
- Reject copying `%t18` or any successor/join-block compare result on a
  predecessor edge.
- Reject consuming `load_from_stack_slot` when the record is
  `missing_stack_freshness`.
- Reject testcase-shaped fixes keyed to `20010329-1`, `%t17`, `%t18`, `%t22`,
  one stack slot, or one block name.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, or baseline/log churn.
