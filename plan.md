# AArch64 Codegen Markdown Shard Reconciliation Runbook

Status: Active
Source Idea: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Activated after: ideas/closed/224_common_mir_container_and_target_printer_boundary.md

## Purpose

Reconcile the legacy markdown shards under
`src/backend/mir/aarch64/codegen/*.md` against the current post-228 and
post-224 AArch64 backend architecture.

Goal: classify every listed codegen markdown shard as already represented by
compiled code, stale legacy evidence to retire, or a real missing-feature shard
that needs a focused follow-up route.

## Core Rule

Do not mechanically convert markdown shards into same-named C++ files. Treat
the current compiled backend and shared MIR printer boundary as authoritative,
and use each markdown file only as evidence to classify.

## Read First

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `ideas/closed/228_aarch64_module_phoenix_drafts_to_implementation.md`
- `ideas/closed/224_common_mir_container_and_target_printer_boundary.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/returns.cpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `src/backend/mir/aarch64/codegen/traversal.cpp`
- `src/backend/mir/aarch64/codegen/*.md`

## Current Targets

- durable shard classification ledger
- current compiled AArch64 codegen owner mapping
- stale-shard retire rationale
- focused follow-up ideas or bounded implementation routes for real missing
  features
- proof references where existing code or tests already demonstrate coverage

## Non-Goals

- Do not eliminate all markdown files as a goal by itself.
- Do not create same-named `.cpp` / `.hpp` pairs for every markdown shard.
- Do not resurrect legacy record-pile or broad module-emitter ownership.
- Do not convert `records.md` into `records.cpp` / `records.hpp` without a
  later explicit human-approved idea.
- Do not move shared MIR printer-boundary ownership back into AArch64 codegen.
- Do not patch missing BIR or shared MIR facts inside AArch64 codegen.
- Do not weaken tests, mark supported paths unsupported, or claim
  expectation-only progress.
- Do not add named-case shortcuts for a single proof fixture.

## Working Model

- Already-converted shards map to current compiled owners and evidence.
- Stale shards receive a retire rationale and must not drive architecture
  reversal.
- Real missing-feature shards become focused follow-up ideas or bounded routes
  with proof contracts.
- Missing prepared semantic facts belong in BIR, shared MIR, or another
  focused idea, not hidden in target codegen.
- Ledger entries may live in `todo.md` while execution is active, but final
  completion requires every source shard to have a recorded classification.

## Ledger Entry Shape

Use this shape for every reconciled shard:

```text
Shard: src/backend/mir/aarch64/codegen/<name>.md
Classification: already-converted/reconcile-ledger | stale/reject-retire | real-missing-feature
Current Owner: <compiled files, shared MIR boundary, or none>
Review Result: <why the classification is correct>
Proof or Evidence: <existing selector, code reference, or reason no proof applies>
Follow-Up: <none, retire note, or ideas/open draft path to create>
```

## Execution Rules

- Start from compiled code ownership, not markdown filenames.
- Keep each execution packet to a small shard group.
- Check current owners before labeling a shard as missing.
- Prefer ledger clarity over file-count changes.
- If a missing feature is real but broad, create a follow-up idea instead of
  absorbing the repair here.
- Build after any code-changing packet; documentation-only ledger updates do
  not require compile proof.
- Use narrow proof for any easy implementation fix and escalate to broader
  backend checks when shared route or public assembly behavior is touched.

## Ordered Steps

### Step 1: Establish Current Owner Map And Ledger Location

Goal: map the live post-224/post-228 AArch64 codegen surfaces before reviewing
individual markdown shards.

Primary targets:

- `src/backend/mir/aarch64/codegen/`
- `src/backend/mir/aarch64/module/`
- `src/backend/mir/`
- `todo.md`

Actions:

- Inspect compiled codegen owners for ALU, comparison, returns, emit,
  dispatch, instruction records, operands, traversal, and machine printing.
- Confirm where shared MIR printer ownership now lives after idea 224.
- Choose whether the active ledger will be maintained directly in `todo.md` or
  in a separate follow-up artifact referenced from `todo.md`.
- Record the owner map, ledger location, and first proof strategy in
  `todo.md`.

Completion check:

- `todo.md` names the current owner map, ledger location, and initial proof or
  evidence strategy for the first shard group.

### Step 2: Reconcile Active Compiled Surface Shards

Goal: classify shards that appear to overlap existing compiled owners.

Primary shards:

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/emit.md`

Actions:

- Compare each shard against the current compiled owner files.
- Verify semantic coverage instead of assuming filename matches are complete.
- Write ledger entries with owner files and proof or evidence.
- Create focused follow-up notes only for real gaps that remain valid.

Completion check:

- The four active-surface shards have complete ledger entries and no gap is
  classified missing without current-owner review.

### Step 3: Reconcile Route Boundary And Stale Structural Shards

Goal: retire or classify shards that describe old module-emitter or record-pile
architecture.

Primary shards:

- `src/backend/mir/aarch64/codegen/mod.md`
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.md`

Actions:

- Compare each shard with the current module, emit, traversal, and printer
  boundary.
- Treat `records.md` as stale unless review proves a narrow current fact.
- Do not recreate broad record ownership or same-named compiled files.
- Record retire rationale or owner mapping in the ledger.

Completion check:

- Each route-boundary or structural shard has a ledger entry, and stale
  evidence is retired without architecture reversal.

### Step 4: Reconcile ABI, Call, Frame, And Global Shards

Goal: classify ABI-adjacent shards without expanding this route into broad
lowering repair.

Primary shards:

- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/variadic.md`
- `src/backend/mir/aarch64/codegen/globals.md`

Actions:

- Check whether current compiled owners or prepared BIR facts already cover
  each valid shard intent.
- Separate AArch64 codegen gaps from BIR, shared MIR, or prepared-authority
  gaps.
- Create follow-up idea drafts for nontrivial missing mechanisms.
- Avoid test expectation downgrades or named-case call/frame shortcuts.

Completion check:

- ABI, call, frame, and global shards are classified with owner layers and
  follow-up paths for deeper missing mechanisms.

### Step 5: Reconcile Scalar, Memory, Float, And Wide Operation Shards

Goal: classify operation-family shards while preserving semantic ownership
boundaries.

Primary shards:

- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- `src/backend/mir/aarch64/codegen/i128_ops.md`
- `src/backend/mir/aarch64/codegen/f128.md`

Actions:

- Compare each operation family with current ALU, comparison, operand,
  dispatch, and emit behavior.
- Identify whether missing facts belong in AArch64 lowering, BIR, shared MIR,
  or prepared BIR.
- Keep any implementation fix narrow and proof-backed; split deeper repairs
  into follow-up ideas.

Completion check:

- Scalar, memory, float, and wide operation shards have ledger entries and any
  real missing feature has a focused owner and proof contract.

### Step 6: Reconcile Target-Specific Extra Shards

Goal: classify lower-priority target-specific shards without overfitting.

Primary shards:

- `src/backend/mir/aarch64/codegen/atomics.md`
- `src/backend/mir/aarch64/codegen/intrinsics.md`
- `src/backend/mir/aarch64/codegen/inline_asm.md`
- `src/backend/mir/aarch64/codegen/peephole.md`

Actions:

- Check whether each shard describes current compiled behavior, stale
  architecture, or a valid future feature.
- Create follow-up ideas for real missing features that need separate design.
- Do not add one-off lowering shortcuts for isolated cases.

Completion check:

- Target-specific extra shards are classified with no hidden broad repair work
  absorbed into this reconciliation route.

### Step 7: Finalize Reconciliation And Follow-Up Inventory

Goal: make the shard reconciliation complete and reviewable.

Actions:

- Verify every source markdown shard listed in idea 229 has exactly one
  primary classification.
- Verify stale shards have retire rationale.
- Verify already-converted shards map to current compiled owners.
- Verify every real missing-feature shard has a focused follow-up idea, draft,
  or bounded route with proof expectations.
- Run no broad code validation unless code changed; if code changed, run the
  supervisor-selected proof and any required broader backend check.

Completion check:

- The active ledger covers all listed shards, follow-up inventory is
  reviewable, and the source idea can be considered for closure.
