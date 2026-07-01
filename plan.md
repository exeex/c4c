# BIR Semantic GEP Local-Memory Admission Producer Plan

Status: Active
Source Idea: ideas/open/499_semantic_gep_local_memory_admission_producer.md
Activated From: ideas/closed/496_semantic_lir_to_bir_admission_high_impact_cleanup.md

## Purpose

Implement the focused producer route selected by idea 496 Step 2 for the
`62`-row semantic `gep local-memory` admission family.

## Goal

Publish or deliberately reject semantic local-memory GEP admission records using
explicit BIR producer authority, without RV64/MIR reconstructing missing facts.

## Core Rule

Semantic GEP local-memory admission may become available only when source
object, local-address/provenance, element path, offset/layout, range, and
coordinate authority are explicit. Raw LIR/BIR shape, final homes, names,
testcase shape, lowered target behavior, and route-local slots are not
availability evidence.

## Read First

- ideas/open/499_semantic_gep_local_memory_admission_producer.md
- ideas/closed/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
- build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/summary.md
- build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_counts.tsv
- build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_representatives.tsv
- build/agent_state/496_step2_next_owner_selection/decision.md
- build/agent_state/496_step2_next_owner_selection/open_idea_inventory.tsv

## Current Targets

- Inputs:
  - current semantic `gep local-memory` admission rows;
  - existing BIR local-memory, provenance, layout, and coordinate surfaces;
  - representative cases `src/pr44468.c`, `src/pr48571-1.c`,
    `src/pr65956.c`, `src/pr80421.c`, and `src/20000717-4.c`.
- Outputs:
  - classified GEP local-memory subfamilies and first-owner decisions;
  - a narrow semantic GEP admission contract;
  - producer implementation or precise blocker for the first admissible
    subfamily.

## Non-Goals

- RV64/MIR lowering or target reconstruction of missing producer facts.
- Reopening scalar local-load, local-address provenance, checker-input,
  proof-fact, range-proof, or selected-path producers without concrete
  regression evidence.
- Store local-memory, direct-call, scalar/local-memory, memcpy/memset, alloca,
  signature, control-flow, F128, move-bundle, runtime mismatch, global-data, or
  stack-frame work.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 499 owns BIR semantic GEP local-memory admission. It should first classify
the selected 62-row family by producer authority, then implement only the
subfamily whose source object, derivation/provenance, element path, layout, and
coordinates are explicit enough to admit safely.

## Execution Rules

- Preserve fail-closed diagnostics for missing source object, missing
  derivation, missing provenance, aggregate/member boundary, pointer-derived
  runtime provenance, variadic/runtime boundary, unsupported type,
  raw-shape-only evidence, target-only evidence, and coordinate confusion.
- If classification discovers a lower producer prerequisite, record that
  prerequisite and request lifecycle routing instead of implementing RV64
  inference.
- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

- Lifecycle-only proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed
git diff --check
```

## Steps

### Step 1: Classify GEP Local-Memory Admission Rows

Reproduce and classify the 62-row `gep local-memory semantic family` by
producer authority and representative subfamily.

Completion means a durable artifact records row counts, representatives,
available candidate shapes, fail-closed boundary shapes, and the first
implementation or prerequisite owner.

### Step 2: Define The Semantic GEP Admission Contract

Write the narrow contract for the first admissible subfamily, including required
source object, derivation/provenance, element path, layout/range, and coordinate
authority fields.

Completion means the contract names available and unavailable statuses and
states whether implementation can proceed or a lower producer must be routed
first.

### Step 3: Implement Or Route The First Producer Packet

Implement the first safe semantic GEP admission producer packet, or record the
exact lower prerequisite if Step 2 proves implementation is blocked.

Completion means focused backend coverage proves the available path and
representative fail-closed boundaries, or `todo.md` records the lifecycle route
for the lower prerequisite.
