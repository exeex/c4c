# Pointer-Value Memory Provenance Publication Plan

Status: Active
Source Idea: ideas/open/442_pointer_value_memory_provenance_publication.md

## Purpose

Determine whether runtime pointer dereferences can publish concrete prepared
provenance, require a named opaque-compatibility policy, or must remain
fail-closed before any RV64 target consumer work.

## Goal

Classify the `930930-1` pointer-value memory accesses against the idea 438
authority classifier, then implement or route only the producer publication
work that can prove explicit pointer provenance, layout, extent, and range.

## Core Rule

Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority` and
do not infer pointer-value memory provenance, object extent, layout, or range
in RV64 from raw pointer values, integer casts, filenames, function names,
value names, object labels, or dump shape.

## Read First

- ideas/open/442_pointer_value_memory_provenance_publication.md
- build/agent_state/438_step1_pointer_value_memory_audit/audit.md
- build/agent_state/438_step2_pointer_value_authority_contract/contract.md
- build/agent_state/438_step3_pointer_value_authority_coverage/summary.md
- build/agent_state/433_step4_residual_disposition/930930-1.prepared.out
- build/agent_state/433_step4_residual_disposition/930930-1.object.err
- src/backend/prealloc/addressing.hpp
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/prealloc/stack_layout/coordinator.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- Four `930930-1` pointer-value memory accesses currently reporting
  `layout_authority=unknown` and `range_verdict=unknown_compatible`.
- Runtime pointer sources such as `reg1` and `mr_TR - 8`.
- The idea 438 authority classifier:
  `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Any producer path that can publish concrete provenance, complete object
  extent, matching requested range, and `ProvenInBounds`.
- Any explicitly named `OpaqueCompatibility` policy if concrete provenance is
  impossible but the route is semantically owned.

## Non-Goals

- Redefining or weakening the idea 438 authority contract.
- RV64 target-side reconstruction of pointer provenance, layout, extent, or
  range.
- Store-source/global-memory publication, direct-global return/select-chain,
  or terminator/select publication work.
- Reopening pointer-cast movement from idea 429 or selected global object data
  from idea 433.
- Expectation rewrites, unsupported-marker downgrades, allowlists, runtime
  comparison changes, or pass/fail accounting changes.

## Working Model

Idea 438 made unknown pointer-value memory authority explicitly
non-consumable. This plan owns the next producer question: can the prepared
pipeline publish stronger facts for the runtime pointer patterns, should it
define a narrow opaque compatibility exception with coverage, or should those
patterns remain unsupported. RV64 consumer work is out of scope until this
producer authority is explicit.

## Execution Rules

- Start with evidence classification; do not edit RV64 lowering in Step 1.
- Treat `layout_authority=unknown` and `range_verdict=UnknownCompatible` as
  fail-closed unless a named, tested opaque-compatibility predicate is created.
- Preserve the existing classifier as the contract boundary; any exception must
  be explicit and separately covered.
- Add focused producer/prepared tests, not named testcase shortcuts.
- If the evidence splits into a separate producer family, create or request a
  follow-up idea instead of widening this plan.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Re-Audit Pointer-Value Provenance Publication

Inspect the four `930930-1` pointer-value memory accesses from the 438 audit
and classify each runtime pointer source as concrete-provenance candidate,
opaque-compatibility candidate, or intentionally unsupported. Completion means
`todo.md` records a table for each access with source pointer, current
authority, missing producer fact, and selected next packet.

### Step 2: Select Concrete Provenance Or Opaque Policy Packet

Choose one bounded producer packet: publish concrete provenance for a recurring
owned runtime pointer pattern, define and test a narrow opaque-compatibility
policy, or record why the residual must remain fail-closed or split. Completion
means `todo.md` states the contract, rejected adjacent shapes, owned
files/tests, and proof command.

### Step 3: Implement Producer Publication Or Policy Coverage

Apply the smallest producer/prepared change and focused tests for the selected
packet, or create a separate lifecycle idea if the packet is outside this
source idea. Completion means canonical proof passes and `todo.md` records
whether the affected records pass the authority classifier or the named opaque
policy.

### Step 4: Residual Disposition And Close Readiness

Re-check the representative evidence, classify remaining pointer-value memory
blockers, and decide whether this source idea is complete. Completion means the
idea closes, remains active with an exact producer packet, or routes a durable
follow-up before any RV64 target-consumer work.
