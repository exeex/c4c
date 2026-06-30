# Closed-World Formal Pointer Authority Plan

Status: Active
Source Idea: ideas/open/443_closed_world_formal_pointer_authority.md

## Purpose

Build the producer authority prerequisite needed before idea 442 can extend
formal pointer provenance beyond internal-only callees.

## Goal

Classify available function/linkage/callgraph metadata, define when
same-module callsites are complete authority for callee formal pointers, and
add focused producer coverage for accepted closed-world cases and rejected
external-linkage/no-proof cases.

## Core Rule

Do not publish formal pointer provenance for an external-linkage callee from
observed same-module direct callsites alone. Do not weaken
`prepare::prepared_pointer_value_memory_has_proven_authority`.

## Read First

- ideas/open/443_closed_world_formal_pointer_authority.md
- ideas/open/442_pointer_value_memory_provenance_publication.md
- build/agent_state/442_step4_residual_disposition/classification.md
- build/agent_state/442_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/442_step4_residual_disposition/930930-1.prepared.out
- build/agent_state/442_step4_residual_disposition/930930-1.object.err
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/prealloc/addressing.hpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- The external-linkage formal pointer authority gap exposed by `930930-1::f`.
- Same-module computed-address call argument sources, including
  `source_base=@mem source_delta=792`.
- Function/linkage/callgraph metadata that can prove closed-world,
  internal/private, or no-external-caller authority.
- Existing internal-only formal pointer provenance from idea 442, which must be
  preserved as valid progress.

## Non-Goals

- Marking `930930-1` done while its external-linkage formal pointer rows remain
  `layout_authority=unknown` and `range_verdict=unknown_compatible`.
- Publishing external-linkage formal provenance from observed same-module calls
  alone.
- Pointer-delta propagation such as `%mr_TR - 8` before base formal pointer
  provenance is authorized.
- RV64 target-side provenance inference.
- Store-source/global-memory publication, direct-global return/select-chain,
  terminator/select publication, or selected global object-data work.
- Accepting the rejected `test_baseline.new.log` full-suite refresh.

## Working Model

Idea 442's internal-only route is sound. The remaining representative needs a
separate producer proof that same-module callsites are complete for a callee
formal. Step 1 should determine what authority already exists and what must be
added. Only after that authority is covered should idea 442 resume external
formal provenance or pointer-delta propagation.

## Execution Rules

- Start with classification; do not edit implementation in Step 1.
- Keep external-linkage/no-proof callees fail-closed.
- Treat `LirFunction::is_internal` as already covered by idea 442; do not
  regress it.
- If the only available evidence is observed same-module callsites, reject the
  route as insufficient for external linkage.
- Use focused producer/prepared tests for any accepted closed-world authority.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit Closed-World Formal Authority Inputs

Inspect current function linkage, visibility, callgraph, and prepared producer
metadata. Classify which inputs can prove same-module callsites are complete for
formal pointer provenance and which external-linkage shapes must remain
fail-closed. Completion means `todo.md` records an authority table and the
first executable producer packet or exact blocker.

### Step 2: Define The Authority Contract

Write the producer contract for internal/private/closed-world/no-external-caller
formal pointer authority. Include rejected adjacent shapes, especially
external-linkage callees with only observed same-module callsites. Completion
means `todo.md` states required facts, focused tests, and owned implementation
surfaces.

### Step 3: Implement Or Route Focused Producer Coverage

Add the smallest producer/prepared coverage and implementation needed for the
selected authority packet, or split again if the missing metadata belongs to a
different lifecycle initiative. Completion means proof passes and `todo.md`
records whether the authority is available for idea 442 to consume.

### Step 4: Residual Disposition And Close Readiness

Re-check the external-linkage formal provenance blocker, classify any remaining
rows, and decide whether this source idea is complete. Completion means close,
keep active with an exact remaining authority packet, or route a durable
follow-up.
