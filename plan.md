# BIR Semantic Producer High-Impact Cleanup Plan

Status: Active
Source Idea: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md

## Purpose

Turn the post-contract RV64 gcc_torture BIR semantic producer backlog into
focused, producer-owned work without pushing missing facts into RV64 lowering.

## Goal

Classify high-impact `semantic lir_to_bir lowering` failures by BIR producer
family and select the first implementation-ready ordinary C producer packet.

## Core Rule

BIR-owned semantic facts must be repaired in the BIR/semantic producer layer.
Do not recover missing facts in MIR/RV64 from raw shape, value names, testcase
names, or fallback target inference.

## Read First

- ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
- ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
- docs/rv64_gcc_torture_post_contract/current_scan_summary.md
- docs/rv64_gcc_torture_post_contract/failure_bucket_map.md
- docs/rv64_gcc_torture_post_contract/followup_idea_plan.md
- build/agent_state/rv64_gcc_torture_backend_current_log_path.txt
- build/agent_state/rv64_gcc_c_torture_backend_summary.tsv
- build/agent_state/rv64_gcc_c_torture_backend_failed.txt

## Current Target

- Failure diagnostic family:
  - `backend object route requires semantic lir_to_bir lowering`
- Candidate BIR producer families:
  - local-memory load/store/GEP;
  - alloca-derived storage;
  - direct-call argument metadata;
  - memcpy/memset and byte/object-representation writes.
- First packet:
  - establish current row evidence and bucket counts for BIR semantic producer
    failures before selecting implementation.

## Non-Goals

- RV64/MIR recovery from raw BIR shape.
- F128 helper, ABI, or conversion-driven work.
- Runtime mismatch debugging.
- Prepared global/stack-frame infrastructure triage unless it appears as a
  true BIR semantic producer input.
- Broad BIR rewrites without row ownership and fail-closed tests.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

The umbrella already ranked ordinary non-F128 RV64 instruction-fragment work
ahead of lower-priority F128 routes. Many RV64/prepared follow-up ideas have
since been closed. This runbook now returns to the open BIR semantic producer
cleanup source and starts by proving which semantic producer bucket is still
current and broad enough for a focused repair.

## Execution Rules

- Step 1 is classification-only unless it identifies stale or missing evidence
  that can be regenerated without implementation changes.
- Use existing scan artifacts first. If a fresh external scan is needed, record
  it under `build/agent_state/` and keep default CTest/baseline files untouched.
- Any implementation packet selected later must be backed by representative
  rows and focused fail-closed tests.
- Classification-only proof:

```sh
git diff --check
```

- Code-changing proof, when a producer packet is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Current Semantic Producer Failure Evidence

Inspect the current RV64 gcc_torture backend artifacts and collect rows whose
first owner is `semantic lir_to_bir lowering`. Completion means `todo.md`
records the evidence source, row count, representative rows, and any stale or
missing artifact concerns.

### Step 2: Bucket BIR Producer Families

Group the semantic producer rows by producer family: local memory load/store,
GEP, alloca-derived storage, direct-call argument metadata, memcpy/memset, and
byte/object-representation writes. Completion means each bucket has counts,
representatives, owning layer, and rejected adjacent shapes.

### Step 3: Select The First Producer-Owned Packet

Choose the highest-impact ordinary C bucket with coherent implementation
ownership. Completion means `todo.md` records the selected packet, required
contract, fail-closed boundaries, and whether the work should remain in this
source idea or be split into a new implementation idea.

### Step 4: Residual Disposition And Close Readiness

Decide whether idea 422 is complete as high-impact producer cleanup planning,
should stay active for implementation, or should close after creating precise
follow-up ideas. Completion means lifecycle state records the remaining BIR
producer owners and no active plan silently expands into RV64 lowering.
