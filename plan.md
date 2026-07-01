# BIR Semantic Local-Address Provenance Packaging Plan

Status: Active
Source Idea: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Reopened After: ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md

## Purpose

Resume idea 484 after the lower local-array proof chain closed. The original
idea defined the local-address/array-element provenance authority contract and
routed because no lower carrier could satisfy it without raw-shape inference.
The active work is now the packaging packet that exposes production checker
inputs in that contract shape.

## Goal

Publish local-address/array-element provenance records for dynamic local-array
element accesses by consuming `local_array_index_range_checker_inputs`.

## Core Rule

Local-address provenance may be marked available only from matching available
`local_array_index_range_checker_inputs`. Do not infer authority from proof
facts, range certificates, selected paths, interval effects, endpoint bridges,
final homes, route-local slots, lowered index values, testcase shape, names, or
target behavior.

## Read First

- ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
- ideas/closed/486_bir_index_range_proof_path_dominance_carrier.md
- build/agent_state/484_step2_local_address_provenance_contract/contract.md
- build/agent_state/484_step3_local_address_authority_producer/blocker.md
- build/agent_state/484_step4_residual_disposition/disposition.md
- build/agent_state/486_step5_checker_inputs_from_proof_facts/summary.md
- build/agent_state/486_step6_residual_disposition_after_checker_inputs/disposition.md

## Resumed State

- Idea 484 owns the local-address/array-element provenance authority contract.
- Idea 486 now publishes `local_array_index_range_checker_inputs` from matching
  production `local_array_proof_facts`.
- Reopened 484 should package checker inputs into the Step 2 record shape and
  leave scalar-load consumption for the next source idea.

## Current Targets

- Inputs:
  - dynamic local-array element path and derivation records;
  - `local_array_index_range_checker_inputs` records from idea 486;
  - existing local-address/array-element provenance record surfaces from idea
    484, or a narrowly added equivalent if the surface was only specified.
- Outputs:
  - available local-address/array-element provenance records for clean bounded
    dynamic local-array rows;
  - fail-closed records for missing checker inputs, unavailable checker inputs,
    missing source object, missing derivation, missing dynamic index, missing
    range, out-of-bounds, aggregate/member, integer-pointer round-trip, global,
    variadic/runtime, unsupported type, bootstrap, raw-shape-only, and
    target-only cases;
  - residual disposition for whether idea 483 scalar local-load production can
    resume.

## Non-Goals

- Recomputing proof facts, range certificates, selected-path coverage,
  dominance/guard validity, or same-value/no-clobber interval facts.
- Consuming `local_array_proof_facts` or `local_array_index_range_proofs`
  directly as packaging evidence.
- Scalar local-load production, RV64/MIR lowering, object emission, or broad
  generic provenance analysis.
- Accepting integer-pointer round trips, global provenance, aggregate/member
  ownership, variadic/runtime ownership, or target-only final-home evidence.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 484 is a packaging layer above the local-array checker input surface. It
should translate available checker inputs and matching local-address derivation
identity into the provenance authority record defined by Step 2, while carrying
unavailable statuses forward instead of flattening them into raw-shape or
unknown-provenance fallbacks.

## Execution Rules

- Match packaging records to checker inputs by element path identity, producer
  lookup key, dynamic index, source object, derivation identity, and consumer
  access identity where those fields exist.
- Preserve `lir_producer_instruction_index` as an LIR producer-site coordinate.
- Do not infer authority from route-local `element_slots`, `base_index`,
  lowered index values, type text, testcase names, value names, dump order,
  final homes, or target fallback behavior.
- Keep packaging separate from scalar-load production and RV64/MIR lowering.
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

### Step 5: Package Local-Address Provenance From Checker Inputs

Publish the Step 2 local-address/array-element provenance authority record by
consuming matching `local_array_index_range_checker_inputs`.

Completion means focused backend coverage proves one clean production-backed
available provenance record and preserves fail-closed behavior for missing
checker input, non-available checker input, missing source object, missing
derivation, missing dynamic index, missing range, out-of-bounds,
aggregate/member, integer-pointer round-trip, global, variadic/runtime,
unsupported type, bootstrap, raw-shape-only, target-only, and coordinate
confusion representatives.

### Step 6: Residual Disposition For Idea 483

Re-probe available and fail-closed representatives after Step 5 and decide
whether idea 483 scalar local-load production can resume, or whether another
provenance packaging owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining lower blocker if 483
cannot resume.
