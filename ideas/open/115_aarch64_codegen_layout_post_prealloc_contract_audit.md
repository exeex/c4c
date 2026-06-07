# 115 AArch64 Codegen Layout Post-Prealloc Contract Audit

## Goal

Re-audit `src/backend/mir/aarch64/codegen` after the recent BIR/prealloc and
call-boundary contract work, then produce focused follow-up ideas for reducing
the AArch64 codegen surface without mixing mechanical layout cleanup with
shared-authority migration.

This idea is analysis-only. It should classify and create follow-up ideas; it
should not move implementation files directly.

## Why This Exists

The original layout classification in idea 20 identified early AArch64
over-splitting and produced follow-up ideas 34 through 39. Those cleanup lines
have since landed, and newer work has also moved or clarified major shared
contracts in BIR/prealloc, including call-planning authority, prepared
publication, aggregate call-boundary facts, and the outgoing stack argument
area.

Even after that work, the current AArch64 codegen tree remains far larger than
the reference ARM layout:

```text
src/backend/mir/aarch64/codegen/*.cpp total: 47794 lines
ref/claudes-c-compiler/src/backend/arm/codegen/* total: 7856 lines
```

Largest current AArch64 owners include:

- `calls.cpp`: 8541 lines
- `memory.cpp`: 5267 lines
- `alu.cpp`: 4364 lines
- `i128_ops.cpp`: 2781 lines
- `comparison.cpp`: 2554 lines
- `f128.cpp`: 2502 lines
- `machine_printer.cpp`: 2182 lines
- `instruction.cpp`: 2080 lines
- `dispatch_edge_copies.cpp`: 1331 lines
- `dispatch.cpp`: 1055 lines
- `dispatch_value_materialization.cpp`: 736 lines
- `dispatch_producers.cpp`: 556 lines

The reference ARM codegen keeps most owner files in the low hundreds of lines.
That does not mean line-count parity is the goal, but the size gap is now a
strong signal that AArch64 still mixes at least three things:

- target-local emission detail;
- helpers that can fold back into reference-style owners;
- semantic or planning authority that should live in shared BIR/prealloc before
  x86 and RISC-V consume it.

## Relationship To Prior Work

Use idea 20 as historical context, not as the final map. This audit must account
for the work landed after idea 20, especially:

- calls consolidation and prepared call-source authority cleanup;
- dispatch prepared-publication contraction;
- BIR/prealloc residue audit and its follow-ups;
- idea 113 aggregate call-boundary audit;
- idea 114 prepared outgoing stack argument area contract.

If a current large owner is large because a closed idea intentionally kept
target-local emission detail there, record that and do not create duplicate
work. If a current large owner still contains shared semantic authority, create
a focused follow-up idea for moving that authority before proposing mechanical
file consolidation.

## In Scope

- Inventory current AArch64 codegen `.cpp/.hpp` surfaces and compare them with
  `ref/claudes-c-compiler/src/backend/arm/codegen`.
- Classify the remaining large owners and helper families into:
  - `move-forward`: shared BIR/prealloc authority should be extracted first;
  - `fold-back`: target-local helper should merge into a reference-style owner;
  - `keep-local`: AArch64-specific emission/ABI/representation detail is a
    justified local owner;
  - `contract-needed`: a high-risk area needs tests or dump visibility before
    cleanup;
  - `phoenix-candidate`: the file is too tangled for safe local patches and
    should be rebuilt through the phoenix markdown-review route.
- Pay special attention to:
  - `calls.cpp`
  - `memory.cpp`
  - `alu.cpp`
  - `comparison.cpp`
  - `f128.cpp`
  - `i128_ops.cpp`
  - `machine_printer.cpp`
  - `instruction.cpp`
  - `dispatch.cpp`
  - `dispatch_edge_copies.cpp`
  - `dispatch_value_materialization.cpp`
  - `dispatch_producers.cpp`
- Identify which `dispatch*` residue should fold into owner files and which
  represents shared prepared/BIR facts that should move forward.
- Produce numbered follow-up ideas for the next cleanup slices.

## Out Of Scope

- Direct implementation edits in `src/backend/mir/aarch64/codegen`.
- Deleting or merging files during this analysis.
- Changing build metadata for translation units.
- Chasing line-count reduction as the success metric.
- Reopening idea 20's closed follow-ups unless a concrete current gap remains.
- Starting x86 or RISC-V codegen implementation.

## Expected Outputs

The closure note should contain:

- a current classification table for the large AArch64 codegen owners;
- a dispatch-family residue map explaining what should fold back, move forward,
  stay local, or receive contract tests first;
- explicit "no new idea" notes for areas already covered by closed work;
- follow-up `ideas/open/` files for only the concrete next slices.

Good follow-up idea shapes include:

- a phoenix-style analysis for a single oversized owner such as `memory.cpp` or
  `calls.cpp` if local cleanup is too risky;
- a focused fold-back idea for a `dispatch_*` helper family that is now purely
  target-local;
- a shared BIR/prealloc extraction idea when AArch64 still owns target-neutral
  semantic/planning facts;
- a contract-visibility idea when cleanup would otherwise remove reviewable
  behavior with no test or dump surface.

## Acceptance And Completion Criteria

- The analysis produces a durable owner classification based on current code,
  not stale pre-113 assumptions.
- Each follow-up idea has a bounded owner, likely files, proof route, and
  testcase-overfit reject signals.
- No implementation, build metadata, or tests are changed directly by this
  analysis.
- The current clean full-suite baseline remains the starting point:

```text
Baseline Commit: 5daac44dc41119b9ea3d2a8c1d91e00da78f6aec
Baseline Regex: <full-suite>
100% tests passed, 0 tests failed out of 3427
```

## Reviewer Reject Signals

- The route performs mechanical file moves while claiming to only classify.
- The route treats every large file as bad without distinguishing shared
  authority, target emission detail, and representation code.
- The route creates vague cleanup ideas that do not name concrete file families
  or proof routes.
- The route ignores idea 20, 113, or 114 closure notes and duplicates already
  closed work.
- The route proposes moving AArch64-specific ABI/emission policy into shared
  BIR/prealloc contracts.
- The route proposes x86/RISC-V implementation before the shared/target
  boundary is made explicit.
