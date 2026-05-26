# AArch64 Codegen Layout Classification

## Goal

Classify the extra AArch64 codegen translation units against the reference ARM
codegen layout, then split follow-up cleanup ideas for either moving authority
forward to BIR/shared prepare or folding target-local helpers back into the
matching owner files.

## Why This Exists

`src/backend/mir/aarch64/codegen` currently has many more `.cpp/.hpp` files
than the reference layout under
`ref/claudes-c-compiler/src/backend/arm/codegen`. Some of that growth is real
semantic debt that should be moved forward into BIR/shared prepare and recorded
as follow-up closure debt. Some of it is just over-splitting inside AArch64 and
should be merged back into files that correspond to the reference owners, such
as `calls`, `memory`, `comparison`, `prologue`, or `returns`.

This idea should not directly perform the consolidation. It should produce a
classification map and numbered follow-up ideas so later execution does not
mix semantic authority migration with mechanical file cleanup.

## In Scope

- Inventory all AArch64 codegen `.cpp/.hpp` files that do not have a clear
  counterpart in the reference ARM codegen layout.
- Classify each extra file or helper family into one of these buckets:
  - move-forward: semantic authority belongs in BIR/shared prepare
  - fold-back: target-local helper should merge into a matching reference-style
    owner file
  - keep-local: AArch64-specific emission or ABI detail justifies a separate
    local owner
  - needs-more-evidence: classification requires a small audit before cleanup
- Use recent closure notes, especially ideas 16 through 19, to identify
  already-known move-forward debt.
- Produce follow-up `ideas/open/` files for each coherent cleanup slice.
- Keep the proposed file layout aligned with the reference ARM owner names
  unless a closure note explicitly justifies a different owner.

## Out Of Scope

- Moving or merging implementation files directly.
- Rewriting AArch64 dispatch, calls, memory, or printer behavior.
- Changing build metadata for codegen translation units.
- Treating line-count reduction alone as success.
- Creating one giant cleanup idea that mixes BIR migration with mechanical
  target-local consolidation.

## Acceptance Criteria

- A durable classification table exists for the extra AArch64 codegen file
  families.
- Each extra `dispatch*`, `calls*`, `memory*`, publication, materialization, or
  bridge file is mapped to move-forward, fold-back, keep-local, or
  needs-more-evidence.
- Follow-up ideas are created with stable numbering for the next cleanup
  slices.
- The follow-up ideas separate semantic authority migration from mechanical
  file consolidation.
- The final close note explains which extra files are expected to disappear,
  which should merge into reference-style owners, and which are intentionally
  retained.

## Reviewer Reject Signals

- A patch performs consolidation while claiming to only classify.
- A patch treats every extra file as bad without checking whether its logic
  should move to BIR/shared prepare first.
- A patch creates vague follow-up ideas that do not name owned file families.
- A patch ignores closure-note evidence that a responsibility already belongs
  outside AArch64.
- A patch proposes a layout that diverges from the reference owner model
  without recording why.

## Close Note

Closed after the Step 6 lifecycle review. The active runbook produced a durable
classification table in `todo.md` before close and created numbered follow-up
ideas for every non-`keep-local` cleanup slice:

- `ideas/open/34_aarch64_store_source_publication_planning.md`
- `ideas/open/35_aarch64_calls_foldback_cleanup.md`
- `ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md`
- `ideas/open/37_aarch64_branch_and_entry_foldback_cleanup.md`
- `ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md`
- `ideas/open/39_aarch64_memory_foldback_after_store_source_planning.md`

Expected move-forward outcome: `memory_store_sources.*` carries
target-neutral producer/source-publication planning debt that should move to
shared prepare, BIR, or MIR planning through idea 34 before any pure AArch64
memory consolidation is accepted.

Expected fold-back outcomes: `calls_byval_aggregates.cpp`,
`calls_common.cpp`, `calls_dispatch_bridge.*`, and `calls_moves.cpp` should
merge into the calls owner; `dispatch_diagnostics.*` and
`dispatch_publication_common.hpp` should fold into dispatch/publication owners;
`comparison_branch_fusion.*` should fold into comparison;
`prologue_entry_formals.cpp` should fold into prologue;
`compatibility_projection.*` should fold into the module compatibility build
boundary; and the target-local residue of `memory_store_sources.*` should fold
into memory only after the semantic store-source planning work lands.

Expected keep-local outcomes: direct reference-owner counterparts and justified
AArch64-only emission/ABI/representation owners remain local. This includes
dispatch orchestration, edge-copy emission, prepared-home lookup/materialization,
value materialization, FP materialization, dynamic-stack emission, instruction
and operand representation, machine printing, traversal, effects, and the
reference-aligned owners such as `calls`, `memory`, `comparison`, `prologue`,
`returns`, `alu`, `asm_emitter`, `atomics`, `cast_ops`, `f128`, `float_ops`,
`globals`, `i128_ops`, `inline_asm`, `intrinsics`, `peephole`, and `variadic`.

Close verification found no implementation, build metadata, test, or root
proof-log changes from this classification plan. The regression guard used the
existing canonical backend logs and passed with 163 tests before and 163 tests
after, no new failures.
