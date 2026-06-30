# No-External-Caller Formal Authority Producer Plan

Status: Active
Source Idea: ideas/open/444_no_external_caller_formal_authority_producer.md

## Purpose

Populate a reliable no-external-caller or equivalent closed-world authority
fact before formal pointer provenance consumes same-module call argument
sources for non-internal callees.

## Goal

Audit available frontend/LIR/module authority, define the exact producer
contract for non-internal closed-world formals, and implement only a proven
`NoExternalCaller` or equivalent authority path with focused positive and
negative coverage.

## Core Rule

Do not publish formal pointer provenance for external-linkage callees from
observed same-module callsites alone. Do not treat visibility/linkage
convenience flags as closed-world authority without a tested contract.

## Read First

- ideas/open/444_no_external_caller_formal_authority_producer.md
- ideas/closed/443_closed_world_formal_pointer_authority.md
- ideas/open/442_pointer_value_memory_provenance_publication.md
- build/agent_state/443_step4_residual_disposition/classification.md
- build/agent_state/443_step4_residual_disposition/evidence_snippets.txt
- build/agent_state/443_step4_residual_disposition/930930-1.prepared.out
- src/backend/bir/lir_to_bir/memory/provenance.cpp
- src/backend/prealloc/addressing.hpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp

## Current Targets

- `FormalPointerAuthorityKind::NoExternalCaller` or an equivalent
  closed-world authority producer.
- Non-internal callee formals whose same-module callsites may or may not be
  complete authority.
- External-linkage/no-proof examples such as `930930-1::f`, which must remain
  fail-closed unless producer authority proves no outside caller can provide
  different pointer values.
- Internal/static behavior from idea 443, which must remain valid.

## Non-Goals

- Publishing external-linkage formal pointer provenance from observed
  same-module callsites alone.
- Weakening `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Pointer-delta propagation such as `%mr_TR - 8` before base formal pointer
  authority is proven.
- RV64 target-side provenance inference.
- Store-source/global-memory publication, direct-global return/select-chain,
  terminator/select publication, or selected global object-data work.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Idea 443 installed the authority carrier and the sound `InternalOnly` route.
This plan decides whether the pipeline can prove a stronger closed-world fact
for non-internal definitions. If it cannot, the result should be an explicit
fail-closed disposition rather than target-side inference or a weakened
authority predicate.

## Execution Rules

- Start with classification; do not edit implementation in Step 1.
- Keep external-linkage/no-proof functions fail-closed by default.
- Require focused positive and negative producer/prepared tests for any
  accepted `NoExternalCaller` or equivalent authority.
- Do not use testcase names, one callsite set, one visibility spelling, or dump
  shape as authority.
- Do not touch `test_baseline.new.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Audit No-External-Caller Authority Inputs

Inspect frontend, LIR, module, linkage, visibility, and callgraph facts
available to the producer. Classify which facts can prove no external caller
and which are insufficient convenience signals. Completion means `todo.md`
records an authority table and the first executable producer packet or exact
blocker.

### Step 2: Define The Producer Contract

Specify the accepted `NoExternalCaller` or equivalent closed-world contract,
including required metadata, rejected adjacent shapes, and focused tests.
Completion means `todo.md` states the contract, owned files/tests, and proof
command.

### Step 3: Implement Or Route Authority Production

Populate the authority field only for proven accepted cases, or split again if
the needed metadata belongs to another initiative. Completion means canonical
proof passes and external-linkage/no-proof cases remain fail-closed.

### Step 4: Residual Disposition And Close Readiness

Re-check the external-linkage formal provenance blocker, classify remaining
rows, and decide whether this source idea is complete. Completion means close,
keep active with an exact remaining producer packet, or route a durable
follow-up before returning to idea 442.
