# BIR Dynamic Local-Array Proof Population From LIR Coordinates Plan

Status: Active
Source Idea: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Reopened After: ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md

## Purpose

Resume idea 489 after the lower path/no-clobber certificate producer closed.
The active work is no longer to discover missing lower owners; it is to
populate dynamic local-array proof facts by consuming the now-production
`local_array_index_range_proofs` certificate surface.

## Goal

Populate real dynamic local-array proof facts for LIR-coordinate-backed
producer rows without re-deriving path coverage, dominance/guard validity, or
same-value/no-clobber evidence.

## Core Rule

Dynamic local-array proof facts may be marked available only from matching
`local_array_index_range_proofs` records. Branch proximity, selected path
availability alone, interval facts alone, endpoint bridges, final homes, target
behavior, raw testcase shape, synthetic effect inputs, and
`lir_producer_instruction_index` reinterpretation remain insufficient.

## Read First

- ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
- ideas/closed/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
- build/agent_state/489_step2_lir_coordinate_proof_population_contract/contract.md
- build/agent_state/489_step3_proof_population_route/route.md
- build/agent_state/489_step4_residual_disposition/disposition.md
- build/agent_state/490_step5_range_proof_from_lower_authorities/summary.md
- build/agent_state/490_step6_residual_disposition_after_lower_authorities/disposition.md

## Resumed State

- Idea 488 exposed durable `lir_producer_*` coordinates and lookup keys.
- Idea 489 previously routed because path/no-clobber certificates were absent.
- Idea 490 now publishes `local_array_index_range_proofs` from lower
  selected-path and interval-effect authorities.
- Idea 489 can resume proof population by consuming those certificates.

## Current Targets

- Inputs:
  - dynamic local-array element path records with available `lir_producer_*`
    coordinates and `address_derivation` role;
  - `local_array_index_range_proofs` records populated by idea 490;
  - existing dynamic local-array proof fact/checker-facing surfaces.
- Outputs:
  - available proof facts for clean bounded dynamic local-array rows;
  - precise unavailable status mapping for certificate failures;
  - residual disposition stating whether idea 486 checker input population can
    resume.

## Non-Goals

- Recomputing selected proof-edge path coverage or interval no-clobber from
  raw control-flow/effect data.
- Reopening closed lower owners 490 through 498 unless a concrete regression
  proves their handback wrong.
- Populating idea 486 checker inputs directly unless this active plan explicitly
  reaches that downstream handoff.
- Idea 484 packaging, scalar local-load consumption, RV64/MIR lowering, or
  broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 489 owns the proof-fact producer layer above the certificate surface.
It should treat `local_array_index_range_proofs` as the lower authority and
translate available certificates into proof facts that downstream checker input
population can consume. It must preserve fail-closed certificate statuses
instead of hiding them behind generic availability.

## Execution Rules

- Preserve `lir_producer_instruction_index` as an LIR producer-site coordinate.
- Match proof facts to certificate records by element path identity, producer
  lookup key, proof identity, dynamic index, and normalized bounds where those
  fields exist.
- Do not infer availability from selected paths, interval effects, endpoint
  bridges, final homes, names, or testcase shape.
- Keep proof fact publication separate from checker input population.
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

### Step 5: Populate Proof Facts From Range Proof Certificates

Wire dynamic local-array proof fact publication to consume matching
`local_array_index_range_proofs`.

Completion means focused backend coverage proves one clean production-backed
available proof fact and preserves fail-closed behavior for missing certificate,
non-available certificate, selected-path-only or interval-only inference,
unsupported boundary, missing coordinate, clobber, alias/phi, unknown effect,
non-covering path, non-dominating/non-guarding proof, and coordinate-confusion
representatives.

### Step 6: Residual Disposition For Idea 486

Re-probe available and fail-closed representatives after Step 5 and decide
whether idea 486 checker input population can resume, or whether another proof
fact owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 486
cannot resume.
