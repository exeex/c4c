# BIR Index Range Proof Checker Input Population Plan

Status: Active
Source Idea: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Reopened After: ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md

## Purpose

Resume idea 486 after the proof-fact producer layer closed. The original idea
added the checker/status carrier with synthetic inputs; the active work is to
populate production checker inputs by consuming `local_array_proof_facts`.

## Goal

Turn available dynamic local-array proof facts into checker inputs for the
existing index range/path-dominance carrier while preserving fail-closed status
mapping.

## Core Rule

Checker inputs may use `local_array_proof_facts` as authority. They must not
infer availability from raw selected paths, interval effects, endpoint bridges,
final homes, `local_array_index_range_proofs` directly, testcase shape, names,
or target behavior.

## Read First

- ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
- ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
- build/agent_state/486_step3_range_proof_path_dominance_carrier/summary.md
- build/agent_state/486_step4_residual_disposition/disposition.md
- build/agent_state/489_step5_proof_facts_from_range_certificates/summary.md
- build/agent_state/489_step6_residual_disposition_after_proof_facts/disposition.md

## Resumed State

- Idea 486 already owns the range proof status vocabulary, checker input
  structure, proof record, and `evaluate_local_array_index_range_proof`.
- Idea 489 now publishes `local_array_proof_facts` from matching production
  `local_array_index_range_proofs`.
- Reopened 486 should bridge proof facts into checker inputs and leave the
  existing checker semantics intact.

## Current Targets

- Inputs:
  - dynamic local-array element path records from idea 485;
  - `local_array_proof_facts` records from idea 489;
  - existing checker input and proof record surfaces from idea 486.
- Outputs:
  - production checker inputs populated from matching available proof facts;
  - available checker records for clean bounded dynamic rows;
  - distinguishable fail-closed statuses when proof facts are missing,
    unavailable, mismatched, or ambiguous;
  - residual disposition for whether idea 484 packaging can resume.

## Non-Goals

- Recomputing selected-path coverage, dominance/guard validity, or
  same-value/no-clobber interval facts.
- Consuming `local_array_index_range_proofs` directly as checker input evidence.
- Reopening closed lower owners 487 through 489 unless a concrete regression
  proves their handback wrong.
- Idea 484 packaging, scalar local-load consumption, RV64/MIR lowering, or
  broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 486 owns the checker input layer and range proof verdict/status carrier.
It should treat `local_array_proof_facts` as the immediate production proof
source, map available proof facts into existing checker inputs, and preserve
unavailable proof-fact statuses rather than replacing them with generic
missing-proof failures.

## Execution Rules

- Match checker inputs to proof facts by element path identity, producer lookup
  key, proof identity, dynamic index, and normalized bounds where those fields
  exist.
- Preserve `lir_producer_instruction_index` as an LIR producer-site coordinate.
- Do not infer availability from selected paths, interval effects, endpoint
  bridges, final homes, branch proximity, names, or testcase shape.
- Keep checker input population separate from idea 484 provenance packaging.
- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

- Lifecycle-only proof:

```sh
git diff --check
```

## Steps

### Step 5: Populate Checker Inputs From Proof Facts

Wire production checker input population to consume matching
`local_array_proof_facts`.

Completion means focused backend coverage proves one clean production-backed
available checker record and preserves fail-closed behavior for missing proof
fact, non-available proof fact, selected-path-only or interval-only inference,
unsupported boundary, missing coordinate, clobber, alias/phi, unknown effect,
non-covering path, non-dominating/non-guarding proof, and coordinate-confusion
representatives.

### Step 6: Residual Disposition For Idea 484

Re-probe available and fail-closed representatives after Step 5 and decide
whether idea 484 packaging can resume, or whether another checker-input owner
remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 484
cannot resume.
