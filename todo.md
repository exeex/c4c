Status: Active
Source Idea Path: ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Audit Inputs And Baseline Map

# Current Packet

## Just Finished

Step 1 - Establish Audit Inputs And Baseline Map completed as an analysis-only
inventory packet.

Current owner-size inventory, counted with `wc -l` on 2026-06-07:

| Area | Files | Total lines | Notes |
| --- | ---: | ---: | --- |
| `src/backend/mir/aarch64/codegen` | 74 | 51871 | C++/header owner surface after ideas 34-39, 113, and 114. |
| `ref/claudes-c-compiler/src/backend/arm/codegen` | 20 | 7856 | Reference Rust layout; much smaller and centered on `emit.rs`, so line parity is not a useful goal. |

Largest current AArch64 owner families, grouped by basename:

| Owner family | Lines | Step 2/3 relevance |
| --- | ---: | --- |
| `calls` | 8665 | Largest owner; must be classified after post-114 prepared outgoing-stack-area consumption, not from stale call-reservation assumptions. |
| `memory` | 5485 | Store-source helper family has already folded back; classify remaining memory bulk as current local emission vs remaining shared fact gaps. |
| `alu` | 4486 | Large target-local arithmetic surface; compare against reference `alu.rs`/`emit.rs` split before assuming move-forward debt. |
| `instruction` | 4033 | Machine-record schema/validation surface; likely target representation, but Step 2 should verify whether any contract visibility is missing. |
| `i128_ops` | 2895 | Large specialized lowering owner; compare against reference `i128_ops.rs` and current call/memory contracts. |
| `comparison` | 2731 | Branch-fusion helper already folded back; classify the current combined owner rather than reopening idea 37. |
| `f128` | 2559 | Large ABI/value transport owner; prior aggregate/HFA work means older missing-contract conclusions need current evidence. |
| `machine_printer` | 2201 | Target machine printing/spelling surface; likely keep-local unless Step 2 finds dump/contract visibility gaps. |
| `cast_ops` | 1757 | Outside Step 2 primary list, but part of current large-tail context. |
| `globals` | 1582 | Outside Step 2 primary list; prior store-source/global publication work may already cover semantic source selection. |
| `variadic` | 1580 | Outside Step 2 primary list; idea 113 found no duplicate aggregate `va_arg` follow-up needed. |

Dispatch-family size notes for Step 3:

| Dispatch family | Lines | Current shape |
| --- | ---: | --- |
| `dispatch_edge_copies` | 1391 | Largest dispatch residue; prior shared-edge/publication work says avoid reintroducing predecessor-depth or BIR-name discovery. |
| `dispatch` | 1082 | Dispatch orchestration/diagnostics owner after old diagnostics fold-back. |
| `dispatch_value_materialization` | 762 | Target-local value materialization residue; compare with prepared value-home facts before classifying. |
| `dispatch_producers` | 642 | Remaining producer/publication helper surface; check whether it is still a justified local helper or fold-back residue. |
| `dispatch_publication` | 435 | Smaller current owner after publication-common fold-back; not in Step 3 primary list but relevant context. |
| `dispatch_lookup` | 226 | Small lookup helper; not primary, but useful context for prepared lookup boundaries. |

Reference comparison:

- Reference ARM's largest owner is `emit.rs` at 1999 lines, followed by
  `peephole.rs` at 1223 lines. Most feature files are far below the AArch64
  large owners: `calls.rs` 263, `memory.rs` 252, `alu.rs` 229,
  `comparison.rs` 75, `f128.rs` 334, and `i128_ops.rs` 322.
- The reference backend centralizes emission differently, while AArch64 has
  explicit machine-record, prepared-publication, and contract-consumer
  surfaces. Treat reference layout as an owner-boundary signal, not as a
  line-count target.

Concise prior-work map:

- Idea 20 created the original layout classification and follow-ups 34-39.
  Its broad conclusion is stale as an inventory, but still useful as a
  non-duplication rule: separate shared authority migration from mechanical
  AArch64 fold-back.
- Idea 34 moved a selected store-source decision out of AArch64 same-block
  rediscovery into shared prepared store-source publication. Do not reopen it
  unless current Step 2 finds a concrete remaining source-choice gap.
- Idea 35 folded `calls_byval_aggregates`, `calls_common`,
  `calls_dispatch_bridge`, and `calls_moves` into the calls owner. These files
  are gone; Step 2 should classify today's `calls.cpp`, not recreate the old
  calls fold-back.
- Idea 36 folded `dispatch_diagnostics` and
  `dispatch_publication_common` into dispatch/publication owners. These files
  are gone; dispatch diagnostics/publication-common split ownership is covered.
- Idea 37 folded `comparison_branch_fusion` and `prologue_entry_formals`.
  These files are gone; comparison/prologue helper split ownership is covered.
- Idea 38 folded `compatibility_projection` into `module_compile.cpp`.
  The standalone files are gone; only module-private compatibility projection
  helpers remain.
- Idea 39 and 39a removed the semantic store-source blocker and then folded
  `memory_store_sources.*` into `memory.cpp`. The old memory helper family is
  gone; remaining memory bulk should be classified on current code evidence.
- Idea 113 audited aggregate call-boundary repairs and created only idea 114.
  It explicitly recorded "no new idea" for frame-slot aggregate address
  materialization, local aggregate pointer operand selection, byval aggregate
  transport, and aggregate `va_arg` home preservation.
- Idea 114 closed on 2026-06-06. Prepared call planning now publishes an
  explicit outgoing stack argument area; AArch64 consumes that shared prepared
  area while retaining target-local `x16`, stack adjustment/restoration, and
  store-order policy.

Stale or covered prior conclusions:

- Stale: any audit note saying AArch64 must derive outgoing stack reservation
  from per-argument stack destinations. Idea 114 made the call-level area an
  explicit prepared fact.
- Covered: old mechanical split-file cleanup for calls, dispatch diagnostics,
  branch/prologue entry helpers, compatibility projection, and memory
  store-source helpers.
- Covered unless current code disproves it: aggregate source selection, byval
  transport, and aggregate `va_arg` home preservation were reviewed by idea
  113 and did not receive new follow-up ideas.
- Still live for classification: the current oversized owners may contain
  target-local representation/emission bulk, missing dump/contract visibility,
  phoenix-worthy tangles, or remaining shared-authority residue, but Step 2
  should prove that from current code rather than pre-113/pre-114 notes.

## Suggested Next

Delegate Step 2: classify the large owner families
`calls.cpp`, `memory.cpp`, `alu.cpp`, `comparison.cpp`, `f128.cpp`,
`i128_ops.cpp`, `machine_printer.cpp`, and `instruction.cpp` against current
responsibility. Start with `calls.cpp` and `memory.cpp` because they have the
largest current footprint and the highest risk of stale prior conclusions from
ideas 113/114 and 34/39.

## Watchouts

- This active idea is analysis-only.
- Do not edit `src/backend/mir/aarch64/codegen`, tests, or build metadata.
- Do not treat line-count reduction as the success metric.
- Do not duplicate closed idea 20 follow-ups without a concrete current gap.
- Current verification found no live references to the old standalone helper
  files from ideas 35-39 except the expected module-private
  `derive_compatibility_projection` helper inside `module_compile.cpp`.
- For `calls.cpp`, account for idea 114 before classifying outgoing stack
  reservation/addressing as shared-authority residue.
- For `memory.cpp`, do not treat the old `memory_store_sources.*` critique as
  current evidence; those files have been deleted and folded back.
- For dispatch-family classification, prior closure notes reject AArch64-local
  rediscovery of prepared/publication facts through predecessor scans, BIR name
  matching, or same-block producer matching.

## Proof

No build/test proof required by the delegated packet because this slice was
analysis-only. Verification performed by read-only inventory and historical
mapping commands; no `test_after.log` was produced for this packet.
