# Edge Dependency-Operand Materialization Authority Plan

Status: Active
Source Idea: ideas/open/454_edge_dependency_operand_materialization_authority.md

## Purpose

Define producer/prepared metadata for dependency operands used by edge
source-producer rematerialization.

## Goal

Classify the `%t17` dependency-operand authority gap from `20010329-1`, define
explicit prepared policy for stack-slot load versus cast rematerialization, and
select only a bounded producer/prepared metadata packet.

## Core Rule

Do not route to RV64 target lowering from current facts. A stack home, object
shape, or raw `inttoptr` source is candidate evidence only; the producer must
publish explicit dependency-operand materialization authority first.

## Read First

- ideas/open/454_edge_dependency_operand_materialization_authority.md
- ideas/closed/453_stack_slot_pointer_select_edge_dependency_materialization.md
- build/agent_state/453_step4_residual_disposition/disposition.md
- build/agent_state/453_step3_stack_slot_pointer_dependency_authority/blocker.md
- build/agent_state/453_step2_stack_slot_pointer_dependency_contract/contract.md
- build/agent_state/453_step1_stack_slot_pointer_dependency_audit/audit.md
- build/agent_state/453_step1_stack_slot_pointer_dependency_audit/20010329-1.prepared.out
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- `20010329-1`, edge `logic.rhs.end.13 -> logic.end.14`.
- Incoming `%t18 -> %t22`, where `%t18 = bir.ule ptr %t15, %t17`.
- Dependency `%t17 = bir.inttoptr i32 %t16 to ptr`.
- `%t17` stack-slot home `slot_id=2 offset=16`.
- `%t16` rematerializable immediate `-2147483643`.
- Missing prepared policy for `load_from_stack_slot` versus
  `rematerialize_cast_from_source` versus fail-closed.

## Non-Goals

- RV64 consumer lowering for stack-slot pointer select-edge materialization.
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

The edge source-producer contract needs a dependency operand to be materialized
before the compare can be rematerialized. `%t17` has two plausible policies:
load the pointer value from its stack slot or rematerialize it from `%t16`
through `inttoptr`. Both require producer-owned authority. This plan creates
or rejects that authority surface before any target consumer is selected.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat stack-home and object metadata as candidate evidence, not authority.
- Keep RV64 consumer, general stack-home branch consumer, pointer-provenance,
  and generic instruction-side work separate.
- Add focused producer/prepared tests for accepted authority and fail-closed
  missing, stale, ambiguous, incoherent, unsupported, or non-placement-safe
  facts.
- Select at most one narrow metadata packet after the contract is explicit.
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

### Step 1: Audit Dependency-Operand Metadata Evidence

Inspect the 453 artifacts for `%t17` and adjacent edge dependency operands.
Record value home, object metadata, cast/source identity, edge placement,
freshness/clobber evidence, current prepared facts, and first missing producer
authority. Completion means `todo.md` contains a bucket table and identifies
whether the first packet should target load policy, cast-rematerialization
policy, shared policy representation, or a routed blocker.

### Step 2: Define Dependency-Operand Authority Contract

Specify the prepared record or equivalent fact required for dependency-operand
materialization, including policy kind, slot/object linkage, width, value type,
freshness, clobber safety, cast source identity, edge placement, and
fail-closed states. Completion means `todo.md` states accepted and rejected
shapes, owned files/tests, and proof command.

### Step 3: Implement Or Route First Producer Metadata Packet

If a coherent producer/prepared metadata packet exists, implement the smallest
semantic change with focused coverage. If the first owner is outside prepared
metadata, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check the `%t17` dependency-operand authority against the Step 3 result,
classify remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining producer metadata
packet, or route durable follow-up work.
