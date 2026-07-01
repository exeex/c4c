# BIR Semantic Local-Memory Scalar Load Producer Plan

Status: Active
Source Idea: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Reopened After: ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md

## Purpose

Resume idea 483 after the local-address provenance prerequisite closed. The
original search found no ordinary scalar local-load representative that was
safe without provenance authority; the active work is now to implement the
scalar local-load producer by consuming `local_array_local_address_provenances`.

## Goal

Publish semantic scalar local-memory load facts for clean local-array element
loads backed by available local-address provenance records.

## Core Rule

Scalar local-load facts may be marked available only from matching available
`local_array_local_address_provenances`. Do not infer local object, element
offset, layout, range, or provenance from checker inputs, proof facts, range
certificates, selected paths, interval effects, endpoint bridges, final homes,
testcase shape, names, or target behavior.

## Read First

- ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
- ideas/closed/484_bir_semantic_local_address_provenance_array_element_authority.md
- build/agent_state/483_step1_corrected_local_load_search/audit.md
- build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv
- build/agent_state/484_step5_local_address_provenance_from_checker_inputs/summary.md
- build/agent_state/484_step6_residual_disposition_after_local_address_provenance/disposition.md

## Resumed State

- Idea 483 owns semantic scalar local-memory load production.
- The first run routed because candidate rows such as `pr38048-1.c` required
  local-address provenance and array-element authority.
- Idea 484 now publishes `local_array_local_address_provenances` from matching
  production checker inputs.
- Reopened 483 should consume provenance records and leave RV64/MIR lowering
  and unrelated local-memory families out of scope.

## Current Targets

- Inputs:
  - local-memory load rows from the semantic producer family;
  - `local_array_local_address_provenances` records from idea 484;
  - existing scalar local-load semantic fact surfaces, or a narrowly added
    equivalent if the surface was only specified.
- Outputs:
  - available scalar local-load facts for clean provenance-backed scalar
    local-array element loads;
  - fail-closed facts for missing provenance, unavailable provenance,
    aggregate/member, integer-pointer round-trip, global, variadic/runtime,
    unsupported type, bootstrap, raw-shape-only, target-only, and coordinate
    confusion cases;
  - residual disposition for whether downstream scalar-load consumers or
    lowering work can resume.

## Non-Goals

- Recomputing local-address provenance, checker inputs, proof facts, range
  certificates, selected-path coverage, or interval no-clobber evidence.
- Consuming `local_array_index_range_checker_inputs`,
  `local_array_proof_facts`, or `local_array_index_range_proofs` directly as
  scalar-load evidence.
- RV64/MIR lowering, object emission, store/call/memcpy/memset producers,
  aggregate/member producer work, variadic/byval/va_arg work, volatile/atomic
  work, complex/vector/F128 work, or broad generic load analysis.
- Expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, runtime-comparison changes, or baseline/log
  churn.

## Working Model

Idea 483 is the scalar local-load producer above the local-address provenance
surface. It should translate matching available provenance records plus scalar
load use identity into semantic local-load facts, while carrying unavailable
provenance statuses forward instead of flattening them into target-only or
raw-shape fallbacks.

## Execution Rules

- Match scalar-load facts to provenance records by function, source object,
  derivation identity, element path, dynamic index, consumer load identity, and
  producer coordinate identity where those fields exist.
- Preserve `lir_producer_instruction_index` as an LIR producer-site coordinate.
- Do not use testcase names, value names, dump order, final homes, lowered
  target homes, or RV64 target behavior as availability evidence.
- Keep scalar-load production separate from RV64/MIR lowering and object
  emission.
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

### Step 2: Produce Scalar Local Loads From Provenance

Publish semantic scalar local-memory load facts by consuming matching
`local_array_local_address_provenances`.

Completion means focused backend coverage proves one clean production-backed
available scalar local-load fact and preserves fail-closed behavior for missing
provenance, non-available provenance, aggregate/member, integer-pointer
round-trip, global source object, variadic/runtime boundary, unsupported type,
bootstrap boundary, raw-shape-only evidence, target-only evidence, and
coordinate confusion representatives.

### Step 3: Residual Disposition For Downstream Consumers

Re-probe available and fail-closed representatives after Step 2 and decide
whether downstream scalar-load consumers or lowering work can resume, or
whether another producer owner remains first.

Completion means `todo.md` records the residual disposition, names the next
idea that should resume, and identifies any remaining blocker if downstream
work cannot resume.
