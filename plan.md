# Dependency-Operand Authority Population Plan

Status: Active
Source Idea: ideas/open/455_dependency_operand_authority_population.md

## Purpose

Populate and expose concrete prepared dependency-operand authority records
using the metadata surface closed by idea 454.

## Goal

Classify and populate the representative `%t17` dependency-operand authority
for `20010329-1`, or record the exact missing producer fact that keeps it
fail-closed, before any RV64 consumer work is selected.

## Core Rule

Do not route to RV64 target lowering from metadata type existence alone.
Target consumers may only be reconsidered after concrete populated authority
records exist and are visible in prepared evidence or focused tests.

## Read First

- ideas/open/455_dependency_operand_authority_population.md
- ideas/closed/454_edge_dependency_operand_materialization_authority.md
- build/agent_state/454_step4_residual_disposition/disposition.md
- build/agent_state/454_step3_dependency_operand_authority_metadata/summary.md
- build/agent_state/454_step2_dependency_operand_authority_contract/contract.md
- build/agent_state/454_step1_dependency_operand_metadata_audit/audit.md
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
- Concrete prepared population/printing for `load_from_stack_slot`,
  `rematerialize_cast_from_source`, or exact fail-closed status.

## Non-Goals

- RV64 consumer lowering for stack-slot pointer select-edge materialization.
- Reworking dependency-operand metadata representation beyond minimal
  population corrections.
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

Idea 454 created a planner/predicate for dependency-operand authority. This
plan asks whether the prepared pipeline can produce concrete records for real
edge dependency operands. The first representative is `%t17`: it may be
eligible for explicit cast rematerialization from `%t16`, explicit stack-slot
load only if freshness/clobber-safety are proven, or fail-closed if the
producer cannot populate either policy.

## Execution Rules

- Start with evidence classification; do not edit implementation in Step 1.
- Treat metadata type existence as insufficient authority for a real edge.
- Keep RV64 consumer, general stack-home branch consumer, pointer-provenance,
  and generic instruction-side work separate.
- Add focused producer/prepared tests and prepared-output evidence for
  populated authority and fail-closed missing/incoherent records.
- Select at most one narrow producer/prepared population packet after the
  audit is explicit.
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

### Step 1: Audit Authority Population Evidence

Inspect the 454 artifacts and current prepared output for `%t17`. Record
available edge identity, dependency operand identity, value home, object
linkage, cast/source identity, freshness/clobber facts, current planner inputs,
prepared printing, and first missing population fact. Completion means
`todo.md` contains a bucket table and identifies whether the first packet is
cast-rematerialization population, stack-load population, printer exposure, or
a routed blocker.

### Step 2: Define Population And Printing Contract

Specify which producer facts populate dependency-operand authority records and
how those records become visible in prepared output or focused tests.
Completion means `todo.md` states accepted and rejected population shapes,
owned files/tests, and proof command.

### Step 3: Implement Or Route First Population Packet

If a coherent producer/prepared packet exists, implement the smallest semantic
change with focused coverage. If the first owner is outside population or
printing, record the split or blocker instead of broadening this source.
Completion means proof passes or canonical lifecycle state records the route
decision.

### Step 4: Residual Disposition And Close Readiness

Re-check representative populated authority records against the Step 3 result,
classify remaining residuals, and decide whether this source idea is complete.
Completion means close, keep active with an exact remaining population packet,
or route durable follow-up work.
