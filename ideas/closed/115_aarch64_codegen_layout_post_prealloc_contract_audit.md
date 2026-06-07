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

## Closure Summary

Closed after the Step 5 lifecycle package. The audit remained analysis-only and
did not change implementation files, tests, or build metadata.

Final owner classifications:

| Owner | Classification | Closure disposition |
| --- | --- | --- |
| `calls.cpp` | `keep-local`, `contract-needed` | No new idea. Old helper split cleanup is covered by idea 35, and outgoing stack argument area authority is covered by idea 114. |
| `memory.cpp` | `keep-local`, `contract-needed` | No new idea. Old store-source helper cleanup is covered by ideas 34, 39, and 39a. |
| `alu.cpp` | `keep-local`, `contract-needed` | No new idea. Current evidence is target scalar opcode, immediate, scratch, and publication emission responsibility. |
| `comparison.cpp` | `keep-local`, `move-forward`, `contract-needed` | Follow-up idea 117 created for fused-compare/current-block publication contract residue. |
| `f128.cpp` | `keep-local`, `contract-needed` | No new idea. Evidence points to specialized f128 transport/runtime-helper ABI machinery. |
| `i128_ops.cpp` | `keep-local`, `contract-needed` | No new idea. Evidence points to specialized i128 pair/lane transport, compare/shift, and runtime-helper ABI machinery. |
| `machine_printer.cpp` | `keep-local` | No new idea. This remains AArch64 target spelling and assembly-line materialization. |
| `instruction.cpp` | `keep-local`, `contract-needed` | No new idea. This remains target machine-record schema, spelling, and validation surface. |

Dispatch-family residue map:

| Owner | Classification | Closure disposition |
| --- | --- | --- |
| `dispatch.cpp` | `keep-local`, `contract-needed` | No new idea. It is central prepared-block orchestration and hook wiring; contract evidence is consumer-side. |
| `dispatch_edge_copies.cpp` | `move-forward`, `contract-needed` | Follow-up idea 116 created for edge-publication, parallel-copy, and producer-context contract residue. |
| `dispatch_value_materialization.cpp` | `keep-local`, `contract-needed` | No new idea. It is target materialization glue that delegates same-block/select-chain lookup to prepared helpers. |
| `dispatch_producers.cpp` | `move-forward`, `contract-needed` | Follow-up idea 116 created for current-block join routing, select-chain producer wrappers, and register clobber/read contract residue. |

Explicit no-new-idea decisions:

- No new `calls.cpp` idea for old helper splits or outgoing stack area.
- No new `memory.cpp` idea for old store-source helper cleanup.
- No new `alu.cpp`, `f128.cpp`, `i128_ops.cpp`, `machine_printer.cpp`, or
  `instruction.cpp` idea because the audit did not find a bounded
  non-duplicative shared-authority or fold-back slice.
- No new `dispatch.cpp`-only or `dispatch_value_materialization.cpp`-only idea.
- No phoenix-candidate idea; the large files have identifiable owner domains.
- No fold-back idea; current residue is contract/shared-authority oriented, not
  mechanical helper-file cleanup.

Created follow-up ideas:

- `ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md`:
  covers dispatch prepared producer, edge-publication, current-block join
  routing, select-chain producer, and register clobber/read contract surface
  residue in `dispatch_edge_copies.cpp` and `dispatch_producers.cpp`.
- `ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md`:
  covers fused compare and current-block publication contract residue in
  `comparison.cpp`.

Baseline note preserved from the source idea:

```text
Baseline Commit: 5daac44dc41119b9ea3d2a8c1d91e00da78f6aec
Baseline Regex: <full-suite>
100% tests passed, 0 tests failed out of 3427
```

Close-time regression guard used the existing matching `test_before.log` and
`test_after.log` AArch64/backend smoke logs with
`--allow-non-decreasing-passed`: 7/7 passed before and 7/7 passed after.

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
