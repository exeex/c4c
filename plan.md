# RV64 Select-Edge Cast Dependency Consumer Plan

Status: Active
Source Idea: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md

## Purpose

Consume populated dependency-operand cast-rematerialization authority in the
RV64 select-edge move materializer.

## Goal

Use the explicit `rematerialize_cast_from_source status=available` record for
the `20010329-1` `%t17` dependency to select a bounded RV64 consumer packet,
while keeping stack-load and successor-copy paths fail-closed.

## Core Rule

Do not infer cast dependency rematerialization from raw `inttoptr`, stack
homes, object metadata, or testcase shape. The RV64 consumer may act only on
populated dependency-operand authority with `policy=rematerialize_cast_from_source`
and `status=available`.

## Read First

- ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
- ideas/closed/455_dependency_operand_authority_population.md
- build/agent_state/455_step4_residual_disposition/disposition.md
- build/agent_state/455_step3_dependency_operand_population/summary.md
- build/agent_state/455_step3_dependency_operand_population/20010329-1.prepared.out
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- `20010329-1`, edge `logic.rhs.end.13 -> logic.end.14`.
- Incoming `%t18 -> %t22`, where `%t18 = bir.ule ptr %t15, %t17`.
- Dependency `%t17 = bir.inttoptr i32 %t16 to ptr`.
- Printed authority:
  `policy=rematerialize_cast_from_source status=available`.
- Cast source `%t16` with `cast_source_home=rematerializable_immediate` and
  `cast_source_imm_i32=-2147483643`.
- Parallel stack-load record remains `status=missing_stack_freshness`.

## Non-Goals

- Producing or printing dependency-operand authority records, closed by idea
  455.
- Consuming `load_from_stack_slot` dependency policies.
- Copying `%t18` or any successor/join-block source result on a predecessor
  edge.
- General stack-home branch operand or condition materialization tracked by
  idea 451.
- Pointer-value memory provenance publication, local/global store publication,
  or generic instruction-side lowering.
- Raw-BIR reconstruction or policy inference from filenames, function names,
  block names, testcase names, object metadata alone, or one prepared dump.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The select-edge compare rematerializer already handles register/immediate
compare operands. The remaining representative needs the RHS dependency `%t17`
materialized first from an explicit cast-source authority record. The consumer
should materialize the cast dependency into a usable register or operand from
the recorded `%t16` source, then use that value while rematerializing the
compare into `%t22`'s destination.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat the populated authority record as the only admissible source for this
  route.
- Keep stack-load, general stack-home branch consumer, pointer-provenance, and
  generic instruction-side work separate.
- Add focused RV64 object tests for accepted explicit cast authority and
  fail-closed missing, unavailable, mismatched, stack-load-only, or successor
  copy cases.
- Select at most one narrow RV64 consumer packet after the audit is explicit.
- Do not touch `test_baseline.new.log`, `test_before.log`, `test_after.log`,
  or `review/`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Cast Dependency Consumer Evidence

Inspect the 455 prepared output and current object failure for `20010329-1`.
Record edge identity, source producer, operand role, dependency value, cast
producer, cast source, source home, selected policy/status, stack-load
fail-closed row, and current RV64 move-bundle failure. Completion means
`todo.md` contains a bucket table and identifies the first bounded consumer
packet or exact blocker.

### Step 2: Define RV64 Cast Dependency Consumer Contract

Specify the RV64 conditions for consuming populated cast-rematerialization
authority, including matching records, operand materialization sequence,
temporary/register requirements, supported cast widths, and fail-closed
adjacent shapes. Completion means `todo.md` states accepted and rejected
shapes, owned files/tests, and proof command.

### Step 3: Implement Or Route First Consumer Packet

If a coherent RV64 consumer packet exists, implement the smallest semantic
change with focused coverage. If the first owner is outside cast dependency
consumption, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check `20010329-1` and focused coverage against the Step 3 result, classify
remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining consumer packet,
or route durable follow-up work.

### Step 5: Suppress Authorized Cast-Dependency Stack Publication Bundle

Consume the Step 4 residual packet for the `before_instruction` stack-slot
publication of the authorized `%t17` cast dependency result. Use only populated
`rematerialize_cast_from_source status=available` dependency-operand authority
and carrier-use proof to suppress or safely consume the owned
`consumer_register_to_stack` publication bundle before the select-edge compare
consumer runs. Completion means focused RV64 object coverage proves the
accepted authorized bundle and fail-closed generic stack-slot moves,
`load_from_stack_slot missing_stack_freshness`, raw `inttoptr`,
successor-copy, scratch-clobber, and unrelated move-bundle cases.

### Step 6: Expose Authorized Before-Instruction Move Bundle To Object Route

Add or route the smallest object traversal surface that lets RV64 object
emission observe the exact `before_instruction` move bundle for the authorized
cast-dependency stack publication. The event must be limited to the owned
`consumer_register_to_stack` publication guarded by populated
`rematerialize_cast_from_source status=available` authority and carrier-use
proof. Completion means the Step 5 suppression/consumption hook is reachable,
while generic before-instruction stack moves, `load_from_stack_slot
missing_stack_freshness`, raw `inttoptr`, successor-copy, scratch-clobber, and
unrelated move bundles remain fail-closed.

### Step 7: Final Residual Disposition And Close Readiness

Re-probe and classify the remaining `20010329-1` object-route
`unsupported_move_bundle_target_shape` now that the explicit cast-dependency
compare consumer and authorized before-instruction stack publication
suppression are both in place. Identify whether an exact idea-456 consumer
packet remains or whether the next owner is a separate later
move-bundle/materialization family. Completion means close 456, keep it active
with one exact remaining cast-dependency consumer packet, or split durable
follow-up work with evidence.
