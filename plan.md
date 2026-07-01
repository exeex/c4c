# BIR Semantic Global/Static GEP Admission Producer Plan

Status: Active
Source Idea: ideas/open/500_semantic_global_static_gep_admission_producer.md
Activated From: ideas/closed/499_semantic_gep_local_memory_admission_producer.md

## Purpose

Follow the residual route from idea 499 by handling the six global/static object
GEP rows separated from the semantic `gep local-memory` family.

## Goal

Publish or deliberately reject semantic global/static GEP admission records
using explicit producer authority, without RV64/MIR reconstructing missing
facts.

## Core Rule

Global/static GEP admission may become available only when global/static source
object identity, layout/range, provenance, relocation/selected-data authority,
and coordinates are explicit. Names, labels, raw shape, final homes, object
files, and target behavior are not availability evidence.

## Read First

- ideas/open/500_semantic_global_static_gep_admission_producer.md
- ideas/closed/499_semantic_gep_local_memory_admission_producer.md
- build/agent_state/499_step1_gep_local_memory_classification/summary.md
- build/agent_state/499_step1_gep_local_memory_classification/classification_counts.tsv
- build/agent_state/499_step1_gep_local_memory_classification/representative_shapes.tsv
- build/agent_state/499_step3_gep_local_memory_producer/summary.md

## Current Targets

- Inputs:
  - six global/static object GEP rows from idea 499 classification;
  - existing BIR/global/static object, selected data, layout, relocation, and
    coordinate surfaces.
- Outputs:
  - row classification and first-owner decision for the global/static GEP
    boundary;
  - a narrow semantic global/static GEP admission contract or precise lower
    prerequisite route;
  - producer implementation only if current authority surfaces are sufficient.

## Non-Goals

- RV64/MIR lowering or target reconstruction of missing producer facts.
- Direct local-object GEPs already completed by idea 499.
- Pointer/formal provenance, runtime/string intrinsics, aggregate/member or
  flexible-layout aliases, variadic routing, store local-memory, direct-call,
  scalar/local-memory, memcpy/memset, alloca, F128, move-bundle, runtime
  mismatch, stack-frame, or scalar-load work.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 500 owns only global/static GEP admission. It should first determine
whether current producer surfaces expose enough global/static object authority.
If not, it should route to the exact lower producer prerequisite instead of
adding RV64 target inference.

## Execution Rules

- Preserve fail-closed diagnostics for missing global/static source object,
  missing layout/range, missing provenance, relocation/selected-data gaps,
  raw-shape-only evidence, target-only evidence, and coordinate confusion.
- If classification discovers a lower producer prerequisite, record that
  prerequisite and request lifecycle routing before implementation.
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

### Step 1: Classify Global/Static GEP Rows

Reproduce and classify the six global/static object GEP rows separated by idea
499.

Completion means a durable artifact records representatives, available
candidate shapes, fail-closed boundary shapes, and the first implementation or
prerequisite owner.

### Step 2: Define Or Route The Global/Static GEP Contract

Define the narrow semantic global/static GEP admission contract if current
producer surfaces are sufficient, or route to the missing lower producer.

Completion means the contract or route names required authority fields,
available and unavailable statuses, and whether implementation can proceed.

### Step 3: Implement Or Route The First Global/Static GEP Packet

Implement the first safe semantic global/static GEP producer packet, or record
the exact lower prerequisite if Step 2 proves implementation is blocked.

Completion means focused backend coverage proves the available path and
representative fail-closed boundaries, or `todo.md` records the lifecycle route
for the lower prerequisite.
