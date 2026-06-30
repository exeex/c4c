# Closed-World No-External-Caller Metadata Source Plan

Status: Active
Source Idea: ideas/open/445_closed_world_no_external_caller_metadata_source.md

## Purpose

Find or introduce a real closed-world/no-external-caller metadata source before
`FormalPointerAuthorityKind::NoExternalCaller` can be populated for
non-internal definitions.

## Goal

Decide whether the current compiler model can own a tested closed-world source
for non-internal functions, define the metadata contract for that source, and
only then feed no-external-caller formal authority.

## Core Rule

Do not infer closed-world authority from observed same-module direct calls,
visibility spelling, `LinkNameId`, `can_elide_if_unreferenced`, absent extern
declarations, or testcase shape. The metadata source must account for
function-address escape and indirect-call target exclusion.

## Read First

- ideas/open/445_closed_world_no_external_caller_metadata_source.md
- ideas/closed/444_no_external_caller_formal_authority_producer.md
- ideas/open/442_pointer_value_memory_provenance_publication.md
- build/agent_state/444_step1_no_external_caller_audit/audit.md
- build/agent_state/444_step2_no_external_caller_contract/contract.md
- src/frontend/hir/hir_ir.hpp
- src/codegen/lir/ir.hpp
- src/codegen/lir/hir_to_lir/hir_to_lir.cpp
- src/codegen/lir/hir_to_lir/call/target.cpp
- src/backend/bir/lir_to_bir/module.cpp
- src/backend/bir/bir.hpp

## Current Targets

- A compiler-owned metadata source for no-external-caller or equivalent
  closed-world authority.
- Required metadata for function identity, definition status, complete caller
  set, function-address escape, indirect-call target exclusion, and any
  visibility/linker contract.
- `FormalPointerAuthorityKind::NoExternalCaller` as a downstream consumer of
  that source, not as the source itself.
- External-linkage/no-proof functions such as `930930-1::f`, which must remain
  fail-closed unless the new metadata source proves closed-world authority.

## Non-Goals

- Publishing formal pointer provenance from observed same-module calls alone.
- Treating hidden/protected visibility, `LinkNameId`,
  `can_elide_if_unreferenced`, or absent extern declarations as authority
  without a tested contract.
- Weakening `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Pointer-delta propagation such as `%mr_TR - 8` before base formal authority
  is proven.
- RV64 target-side provenance inference.
- Accepting or modifying `test_baseline.new.log`.

## Working Model

Ideas 443 and 444 established the formal authority carrier and the
false-by-default no-external-caller contract. This plan must answer the missing
source question. If the compiler has no sound source today, the correct result
is a documented rejection or a narrower source idea, not a speculative
implementation.

## Execution Rules

- Start with metadata-source classification; do not edit implementation in
  Step 1.
- Require positive and negative tests before any source feeds
  `NoExternalCaller`.
- Include address-taken, exported function pointer storage, and unresolved
  indirect-call cases in the source contract.
- Keep `930930-1::f` fail-closed unless the selected source proves no outside
  caller can provide different pointer values.
- Do not touch `test_baseline.new.log`.
- Classification-only proof: `git diff --check`.
- Code/test proof, if implementation is selected:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_'
git diff --check
```

## Steps

### Step 1: Classify Possible Metadata Sources

Audit current frontend, HIR, LIR, module, linker/visibility, callgraph, and
whole-program mode surfaces. Classify each possible source as accepted,
rejected, missing, or split-needed for no-external-caller authority. Completion
means `todo.md` records the source table and the first executable packet or
exact blocker.

### Step 2: Define Selected Source Contract Or Rejection

For an accepted source, define the metadata fields, producer ownership, positive
and negative test cases, and downstream consumer path. If no source is sound in
the current compiler model, record the rejection and required future source.
Completion means `todo.md` states whether implementation may proceed.

### Step 3: Implement Source Or Split

If a bounded source exists, implement the metadata publication and focused
coverage, then feed `FormalPointerAuthorityKind::NoExternalCaller` only from
that source. If the source requires a broader compiler mode or analysis, create
or request a separate lifecycle idea. Completion means proof passes or the
split is canonical.

### Step 4: Residual Disposition And Close Readiness

Re-check whether no-external-caller authority can be consumed by the formal
pointer provenance path. Close, keep active with an exact source packet, or
route a durable follow-up before returning to idea 442.
