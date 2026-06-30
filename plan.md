# Stack-Slot Pointer Select-Edge Dependency Materialization Plan

Status: Active
Source Idea: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md

## Purpose

Decide whether select-edge source-producer rematerialization can safely consume
pointer dependency operands with stack-slot homes.

## Goal

Classify the `20010329-1` `%t18/%t17/%t22` stack-slot pointer dependency,
define the prepared authority required to materialize it at a predecessor-edge
move site, and select only a bounded semantic packet that preserves existing
register/immediate rematerialization behavior.

## Core Rule

Do not copy `%t18` from the successor/join block on the predecessor edge, and
do not load or rematerialize `%t17` from a stack slot unless explicit prepared
facts prove slot identity, width, freshness, clobber safety, edge placement,
and target compatibility.

## Read First

- ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
- ideas/closed/452_select_edge_source_producer_rematerialization.md
- ideas/open/451_stack_home_branch_operand_materialization.md
- build/agent_state/452_step4_residual_disposition/disposition.md
- build/agent_state/452_step3_select_edge_rematerialization/summary.md
- build/agent_state/452_step2_rematerialization_contract/contract.md
- build/agent_state/452_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/452_step4_residual_disposition/20010329-1.object.err
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` current `unsupported_move_bundle_target_shape` residual.
- `logic.rhs.end.13 -> logic.end.14`, incoming `%t18 -> %t22`.
- `%t18 = bir.ule ptr %t15, %t17` defined in the successor/join block.
- `%t17 = bir.inttoptr i32 %t16 to ptr`, with `%t16` rematerializable
  immediate `-2147483643` and `%t17` stack-slot home `slot_id=2 offset=16`.
- Prepared stack-slot pointer dependency facts needed to decide whether the
  compare operand can be loaded, rematerialized from source, or must remain
  fail-closed.

## Non-Goals

- Reopening direct select-result branch publication closed by idea 450.
- Reopening register/immediate select-edge rematerialization closed by idea
  452.
- General branch consumer stack-home operand/condition materialization tracked
  by idea 451.
- Pointer-value memory provenance publication, local/global store publication,
  or generic instruction-side lowering.
- Raw-BIR reconstruction or shape matching from filenames, function names,
  block names, testcase names, or one prepared dump layout.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

The selected edge needs a compare rematerialized into `%t22`'s destination
register. Existing support covers compare operands that are register or
immediate compatible. `%t17` is different: it is a pointer dependency with a
stack-slot home. This plan decides whether prepared facts can safely authorize
loading that pointer from the stack slot, rematerializing it from its `inttoptr`
source, or keeping the route fail-closed until a producer/home contract exists.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw `inttoptr` and stack-slot spelling as candidate evidence only, not
  authority.
- Keep general stack-home branch consumers, pointer provenance, and generic
  instruction-side residuals separate unless they are explicitly required by
  the selected select-edge dependency contract.
- Add focused tests for accepted stack-slot pointer dependency authority and
  fail-closed missing, stale, ambiguous, unavailable, or unsupported facts.
- Select at most one narrow code packet after the contract is explicit.
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

### Step 1: Audit Stack-Slot Pointer Dependency Evidence

Inspect the 452 Step 2 through Step 4 artifacts for `20010329-1`. Record the
edge, incoming value, compare producer, `%t17` cast producer, stack-slot home,
slot metadata, source immediate, edge placement, target register, current
prepared authority, and first missing fact. Completion means `todo.md`
contains a bucket table and identifies whether the first owner is stack-slot
load authority, cast rematerialization authority, producer/home metadata, or a
different routed blocker.

### Step 2: Define Stack-Slot Pointer Dependency Contract

Specify the prepared facts required to materialize a stack-slot pointer
dependency at a predecessor-edge select move, including slot identity, width,
freshness, clobber safety, value type, load/rematerialization policy, edge
placement, and target constraints. Completion means `todo.md` states accepted
and rejected shapes, owned files/tests, and proof command.

### Step 3: Implement Or Route First Stack-Slot Dependency Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is missing
producer/home authority, pointer provenance, general stack-home branch
materialization, or generic instruction lowering, record the split or blocker
instead of broadening this source. Completion means proof passes or canonical
lifecycle state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check the `%t18/%t17/%t22` row against the Step 3 result, classify
remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining stack-slot pointer
dependency packet, or route durable follow-up work.
