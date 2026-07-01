# Dynamic Local-Array LIR Producer Path/No-Clobber Certificate Plan

Status: Active
Source Idea: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Activated From: ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md

## Purpose

Produce the path/no-clobber certificate needed before dynamic local-array proof
population can feed real facts into the idea 486 checker.

## Goal

Publish durable path coverage, dominance/guard, and dynamic-index same-value
certificates keyed to `lir_producer_*` coordinates.

## Core Rule

Do not infer path coverage or no-clobber from branch proximity, loop shape,
value names, testcase names, dump order, final homes, or target behavior. The
certificate must be keyed to the LIR producer site and explicit proof source.

## Read First

- ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
- ideas/closed/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
- build/agent_state/489_step3_proof_population_route/route.md
- build/agent_state/489_step4_residual_disposition/disposition.md

## Current Target

- Missing certificate fields:
  - proof branch/compare identity and selected edge/outcome;
  - path coverage from proof source to `lir_producer_*`;
  - dominance or guard validity;
  - dynamic index same-value/no-clobber interval facts;
  - modeled effects for assignments, phi/alias, calls/helpers, inline asm,
    publications, move bundles, and parallel copies.
- First packet:
  - audit current prepared/BIR path and effect surfaces for a bounded
    certificate producer.

## Non-Goals

- Populating idea 486 checker inputs directly.
- Idea 484 packaging.
- Scalar local-load consumption.
- RV64/MIR lowering.
- Prepared traversal/BIR instruction coordinate conversion.
- Broad generic range analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.
- Touching `review/`, `test_before.log`, `test_after.log`,
  `test_baseline.log`, or `test_baseline.new.log`.

## Working Model

Idea 489 proved proof population has candidate proof sources and LIR producer
binding keys, but lacks a durable certificate tying proof source to producer
site and dynamic-index interval validity.

## Execution Rules

- Step 1 is audit/classification unless an already-bounded certificate surface
  exists.
- Any implementation packet must publish certificate facts or explicit
  unavailable statuses and must not mark checker inputs available directly.
- Classification-only proof:

```sh
git diff --check
```

- Code-changing proof:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit LIR Producer Certificate Inputs

Inspect prepared branch/compare facts, dominance/reachability surfaces, and
effect/no-clobber evidence for dynamic indices keyed to `lir_producer_*`.
Completion means `todo.md` records available certificate inputs and missing
lower-level facts.

### Step 2: Define Path/No-Clobber Certificate Contract

Define the certificate shape and unavailable statuses. Completion means the
contract names proof-source, selected edge/outcome, path coverage,
dominance/guard validity, no-clobber/effect modeling, and coordinate-confusion
rejections.

### Step 3: Implement Or Route Certificate Producer

Implement the bounded certificate packet if Step 2 identifies one. If current
data cannot prove path coverage or no-clobber without inference, record the
exact lower owner and stop without changing idea 489 population, idea 484,
scalar loads, or RV64 lowering.

### Step 4: Residual Disposition And Close Readiness

Re-probe selected representatives and decide whether 490 is complete, blocked
by another lower-level source, or ready to hand back to idea 489 proof
population.
