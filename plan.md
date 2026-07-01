# Semantic LIR-To-BIR Admission High-Impact Cleanup Plan

Status: Active
Source Idea: ideas/open/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
Resumed After: ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md

## Purpose

Resume the semantic admission cleanup after the scalar local-load producer
closed. The first task is to reclassify high-frequency semantic `lir_to_bir`
failures with `local_array_scalar_local_loads` available, then route remaining
producer gaps without moving semantic responsibility into RV64 lowering.

## Goal

Classify and repair high-impact semantic LIR-to-BIR admission failures while
requiring downstream local-load consumers to use published semantic facts.

## Core Rule

Semantic admission or downstream lowering may consume `local_array_scalar_local_loads`;
it must not reconstruct local object, element offset, layout, range,
provenance, exact-address checks, or scalar load identity from provenance
records, lower proof surfaces, final homes, raw testcase shape, names, or target
behavior.

## Read First

- ideas/open/496_semantic_lir_to_bir_admission_high_impact_cleanup.md
- ideas/closed/483_bir_semantic_local_memory_scalar_load_producer.md
- build/agent_state/483_step2_scalar_local_loads_from_provenance/summary.md
- build/agent_state/483_step3_residual_disposition_after_scalar_local_loads/disposition.md
- build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv
- docs/rv64_gcc_torture_post_contract/failure_bucket_map.md
- docs/rv64_gcc_torture_post_contract/followup_idea_plan.md

## Current Targets

- Inputs:
  - current semantic `lir_to_bir` failure bucket evidence;
  - `local_array_scalar_local_loads` as the completed scalar local-load fact
    surface;
  - open producer/lowering idea inventory.
- Outputs:
  - refreshed semantic-admission classification after the scalar local-load
    producer closed;
  - producer-owned follow-up packets for remaining semantic families;
  - explicit sequencing for any RV64/MIR consumer that can now consume scalar
    local-load facts.

## Non-Goals

- Reopening local-array provenance, checker-input, proof-fact, range-proof, or
  selected-path producers without concrete regression evidence.
- Reconstructing scalar local-load facts inside RV64/MIR lowering.
- Move-bundle materialization, F128/long-double soft-float implementation,
  runtime mismatch triage, or global/stack-frame infrastructure unless the
  refreshed classification proves they are the next owner.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline churn.

## Working Model

Idea 496 is a producer-side cleanup and routing plan. It should use the newly
closed 483 fact surface to separate resolved local-array scalar-load admission
from remaining semantic producer failures. It can then either define the next
producer packet or hand a fact-consuming RV64/MIR idea forward when producer
authority already exists.

## Execution Rules

- Treat `local_array_scalar_local_loads` as the only scalar local-array
  local-load authority for downstream consumers.
- Preserve fail-closed statuses for missing/non-available scalar-load facts and
  for unrelated aggregate/member, variadic, global, runtime, unsupported type,
  bootstrap, raw-shape-only, target-only, and coordinate-confusion families.
- If a remaining failure lacks producer authority, create or select a producer
  idea before any RV64 lowering work.
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

### Step 1: Reclassify Semantic Admission After Scalar Local Loads

Refresh the semantic `lir_to_bir` admission bucket with
`local_array_scalar_local_loads` available and identify which failures are now
fact-consuming downstream work versus still-missing producer authority.

Completion means the handoff directory records refreshed counts,
representative rows, resolved local-array scalar-load coverage, and remaining
first-owner families without RV64 reconstructing semantic facts.

### Step 2: Select The Next Producer Or Consumer Packet

Use Step 1 classification to choose the next implementation packet.

Completion means `todo.md` names the selected next owner, distinguishes
producer gaps from valid consumers of `local_array_scalar_local_loads`, and
records whether lifecycle should switch to a more specific open idea.
