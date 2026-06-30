# Select-Edge Source-Producer Rematerialization Plan

Status: Active
Source Idea: ideas/open/452_select_edge_source_producer_rematerialization.md

## Purpose

Define and consume explicit prepared authority for rematerializing
source-producer dependency chains at predecessor-edge out-of-SSA move sites.

## Goal

Classify the `20010329-1` select-edge move-bundle blocker, define the facts
required to rematerialize successor/join-block compare dependencies on
predecessor edges, and select only a bounded producer or consumer packet that
uses explicit prepared authority.

## Core Rule

Do not copy a successor/join-block temporary on a predecessor edge unless the
value is proven available there. If a source value is not edge-available, it
must be rematerialized from explicit prepared source-producer dependency facts
or remain fail-closed.

## Read First

- ideas/open/452_select_edge_source_producer_rematerialization.md
- ideas/closed/450_select_result_branch_publication.md
- ideas/open/451_stack_home_branch_operand_materialization.md
- build/agent_state/450_step4_residual_disposition/disposition.md
- build/agent_state/450_step3_select_result_branch_publication/blocker.md
- build/agent_state/450_step4_residual_disposition/20010329-1.prepared.out
- build/agent_state/450_step4_residual_disposition/20010329-1.object.err
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/publication_plans.hpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- `20010329-1` current `unsupported_move_bundle_target_shape` residual.
- `main.logic.end.14` select result `%t22` with incoming `%t18`.
- `%t18 = bir.ule ptr %t15, %t17`, where `%t18` is defined in the
  successor/join block and is not directly copyable on the predecessor edge.
- `%t17 = bir.inttoptr i32 %t16 to ptr` with a stack-slot pointer home.
- Prepared source-producer dependency facts needed to decide whether the
  compare/cast chain can be rematerialized at the edge.

## Non-Goals

- Reopening direct select-result branch publication closed by idea 450.
- Stack-home branch operand or condition materialization outside the selected
  rematerialization dependency, tracked by idea 451.
- Pointer-value memory provenance publication.
- Generic instruction-side lowering, including current `20000622-1`.
- Raw-BIR reconstruction or shape matching from filenames, function names,
  block names, testcase names, or one prepared dump layout.
- Expectation rewrites, unsupported downgrades, allowlists, runtime-comparison
  changes, pass/fail accounting changes, or `test_baseline.new.log` changes.
- Touching `review/`, `test_before.log`, or `test_after.log`.

## Working Model

Select-result branch publication can expose predecessor-edge move bundles. A
move bundle may need to materialize an incoming value whose producer lives in
the successor/join block. In that case a register copy from the producer result
is unsound; the edge materializer needs an explicit prepared source-producer
dependency contract describing the operation, operands, homes, placement, and
target constraints for rematerialization.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat raw compare/cast chains as candidate evidence only, not authority.
- Keep stack-home, pointer-provenance, and generic instruction-side residuals
  separate unless they are an explicitly required operand in the selected
  rematerialization contract.
- Add focused tests for accepted rematerialization authority and fail-closed
  missing, incoherent, unavailable, or unsupported dependency chains.
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

### Step 1: Audit Select-Edge Rematerialization Evidence

Inspect the 450 Step 3 and Step 4 artifacts for `20010329-1`. Record each
candidate edge, incoming value, producer instruction, dependency operand,
value home, edge availability, current prepared source-producer fact, and
first missing authority. Completion means `todo.md` contains a bucket table
and identifies the first bounded rematerialization packet or exact blocker.

### Step 2: Define Source-Producer Rematerialization Contract

Specify the prepared facts required to rematerialize source-producer
dependencies at predecessor-edge move sites, including operation identity,
operand homes, placement or dominance proof, edge availability, supported
dependency shapes, and target register/storage constraints. Completion means
`todo.md` states the accepted and rejected shapes, owned files/tests, and proof
command.

### Step 3: Implement Or Route First Rematerialization Packet

If a coherent producer/prepared or RV64 consumer packet exists, implement the
smallest semantic change with focused coverage. If the first owner is
stack-home materialization, pointer provenance, generic instruction lowering,
or missing producer authority, record the split or blocker instead of
broadening this source. Completion means proof passes or canonical lifecycle
state records the route decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative select-edge rematerialization rows against the Step 3
result, classify remaining residuals, and decide whether this source idea is
complete. Completion means close, keep active with an exact remaining
rematerialization packet, or route durable follow-up work.
