# Dynamic Local-Array LIR Producer Path/No-Clobber Certificate Plan

Status: Active
Source Idea: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Reopened After: ideas/closed/494_dynamic_local_array_lir_producer_interval_effect_classifier.md

## Purpose

Resume idea 490 after the lower selected-path and interval-effect owners
closed. The active work is now implementation of the path/no-clobber
certificate surface that idea 489 routed here.

## Goal

Publish dynamic local-array range proof certificates keyed to LIR producer
coordinates by consuming the selected proof-edge path record and interval
effect fact surfaces.

## Core Rule

`available` range proof facts may be emitted only when a matching selected
proof-edge path record and a matching interval effect record are both
`available` for the same dynamic local-array producer key, proof source, and
dynamic index. Selected path availability alone, interval evidence alone,
endpoint bridges alone, final homes, target behavior, raw testcase shape, and
synthetic effect inputs remain insufficient.

## Read First

- ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
- ideas/closed/491_dynamic_local_array_selected_proof_edge_path_certificate.md
- ideas/closed/492_dynamic_local_array_selected_proof_edge_path_record_status_api.md
- ideas/closed/493_dynamic_local_array_selected_proof_edge_path_record_collector_population.md
- ideas/closed/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
- build/agent_state/494_step6_residual_disposition_after_stored_stream/disposition.md

## Resumed State

- Original idea 490 completed as a routed investigation and identified missing
  lower owners.
- Closed ideas 491 through 494, plus supporting ideas 497 and 498, now provide
  the selected path, endpoint bridge, ordered effect-source stream, and
  interval effect surfaces needed for real certificate publication.
- The old `LocalArrayIndexRangeProofInputs::no_clobber_known` style must not
  be treated as sufficient production evidence for dynamic rows.

## Current Targets

- Inputs:
  - dynamic local-array producer rows keyed by `lir_producer_lookup_key`;
  - `local_array_selected_proof_edge_paths`;
  - `local_array_interval_effects`;
  - existing local-array range proof status vocabulary.
- Outputs:
  - production-backed available range proof records for clean bounded dynamic
    local-array rows;
  - precise unavailable statuses for missing selected path, missing interval
    effect, non-covering path, non-dominating/non-guarding proof, missing
    no-clobber, clobber, alias/phi, unknown effect, missing coordinates,
    unsupported boundary, and coordinate confusion;
  - residual disposition stating whether idea 489 proof population can resume.

## Non-Goals

- Reopening closed lower owners 491 through 494, 497, or 498 unless a concrete
  regression proves their handback wrong.
- Recomputing selected path, endpoint bridge, ordered effect-source, or
  interval effect evidence from raw testcase shape.
- Populating idea 489 proof facts directly.
- Populating idea 486 checker inputs directly.
- Idea 484 packaging, scalar local-load consumption, or RV64/MIR lowering.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 490 is the certificate bridge between lower path/effect authority and
proof population. It should convert matching lower records into the existing
dynamic local-array range proof surface, preserving fail-closed statuses so
idea 489 can consume one explicit certificate family instead of re-deriving
path coverage or no-clobber.

## Execution Rules

- Preserve `lir_producer_instruction_index` as a LIR producer-site coordinate;
  do not reinterpret it as prepared traversal order or BIR instruction index.
- Match lower records by exact producer key and compatible proof/dynamic-index
  identity; do not use fuzzy block, value-name, or dump-order matching.
- Keep path failures and effect failures distinguishable when mapping into the
  range proof status surface.
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

### Step 5: Publish Range Proof Certificates From Lower Authorities

Wire dynamic local-array range proof publication to consume matching
`local_array_selected_proof_edge_paths` and `local_array_interval_effects`.
Publish `Available` only for rows backed by both lower available records.

Completion means focused backend coverage proves one production-backed
available certificate and preserves fail-closed behavior for missing selected
path, selected-path-only evidence, missing interval effect, interval-only
evidence, clobber, alias/phi, unknown effect, missing coordinate, unsupported
boundary, non-covering path, non-dominating/non-guarding proof, and coordinate
confusion representatives.

### Step 6: Residual Disposition For Idea 489

Re-probe available and fail-closed representatives after Step 5 and decide
whether idea 489 proof population can resume, or whether another lower
certificate owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 489
cannot resume.
