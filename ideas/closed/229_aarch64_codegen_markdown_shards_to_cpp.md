# AArch64 Codegen Markdown Shard Reconciliation

Status: Closed
Created: 2026-05-14
Closed: 2026-05-14
Parent Context: ideas/closed/228_aarch64_module_phoenix_drafts_to_implementation.md

## Intent

Review and reconcile the legacy markdown shards under
`src/backend/mir/aarch64/codegen/*.md` against the current post-228 AArch64
backend architecture. This is no longer a mechanical route to convert every
markdown file into a same-named `.cpp` / `.hpp` pair.

The post-228 codegen surface already has compiled owners such as
`alu.cpp`, `comparison.cpp`, `returns.cpp`, `emit.cpp`,
`dispatch.cpp`, `instruction.cpp`, `machine_printer.cpp`, `operands.cpp`, and
`traversal.cpp`. Reconciliation should decide whether each markdown shard is:

- already represented by current compiled code and only needs a durable ledger
  entry
- stale legacy evidence that should be rejected, retired, or archived as
  non-authoritative context
- a real missing-feature shard that deserves a focused implementation idea or
  bounded execution route

Idea 224 owns the shared MIR printer boundary and any active migration noise
around that boundary. This idea must not reopen that ownership while 224 is
active; it should only classify codegen markdown evidence against the current
layout and preserve clear follow-up intent.

## Current Context

Idea 228 is closed and supplied the current architecture this review should use
as its baseline. The old markdown shards remain useful only as evidence to be
checked against the live compiled route, not as an implementation blueprint to
restore wholesale.

`src/backend/mir/aarch64/codegen/` currently includes active compiled surfaces
for selected-machine-node lowering, dispatch, operands, instruction modeling,
emission, returns, comparison, ALU handling, traversal, and machine printing.
Those surfaces should be treated as the first source of truth before any
markdown shard is labeled missing.

## Source Markdown Set

This idea owns review of these source artifacts:

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/asm_emitter.md`
- `src/backend/mir/aarch64/codegen/atomics.md`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/emit.md`
- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- `src/backend/mir/aarch64/codegen/globals.md`
- `src/backend/mir/aarch64/codegen/i128_ops.md`
- `src/backend/mir/aarch64/codegen/inline_asm.md`
- `src/backend/mir/aarch64/codegen/intrinsics.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/mod.md`
- `src/backend/mir/aarch64/codegen/peephole.md`
- `src/backend/mir/aarch64/codegen/prologue.md`
- `src/backend/mir/aarch64/codegen/records.md`
- `src/backend/mir/aarch64/codegen/returns.md`
- `src/backend/mir/aarch64/codegen/variadic.md`

## Reconciliation Categories

Each shard should receive exactly one primary classification.

### Already-Converted / Reconcile-Ledger

Use this category when the shard's valid intent is already represented by
compiled post-228 surfaces. The work is to write a ledger entry that maps the
markdown evidence to the owning compiled files and, when useful, the existing
tests or selectors that prove the behavior.

Likely examples include:

- `alu.md` -> `alu.cpp` / `alu.hpp`
- `comparison.md` -> `comparison.cpp` / `comparison.hpp`
- `returns.md` -> `returns.cpp` / `returns.hpp`
- `emit.md` -> `emit.cpp` / `emit.hpp`

The exact mapping must be verified during execution; do not assume a filename
match means the semantic coverage is complete.

### Stale / Reject-Retire

Use this category when the shard describes legacy ownership, record piles,
module-emitter structures, or implementation shape that conflicts with the
current backend architecture. These shards should be retired as stale evidence,
not converted back into old surfaces.

`records.md` belongs here unless review proves there is a narrowly valid
current fact to preserve. Treat it as stale legacy record-pile evidence to
retire, not as a reason to recreate `records.cpp` / `records.hpp` or restore
deleted broad record ownership.

### Real Missing-Feature Shard

Use this category when a shard describes a still-valid AArch64 backend feature
that current compiled code does not cover. The output should be a focused
implementation idea, draft, or bounded runbook slice with its own proof
contract. Do not smuggle a broad feature family into this reconciliation route.

If the missing feature depends on facts that should be prepared by BIR or the
shared MIR route, write a separate idea for that dependency instead of filling
the semantic gap inside AArch64 codegen.

## Review Rule

For each markdown shard:

1. Compare the shard against the current compiled codegen layout and the
   closed idea 228 architecture.
2. Check whether any apparent gap is already owned by active compiled surfaces
   or by idea 224's shared MIR printer boundary.
3. Assign one of the three reconciliation categories.
4. Record the rationale, current owner files, and any proof or follow-up idea
   needed.
5. Only propose implementation work for real missing-feature shards with a
   current semantic owner and a focused proof path.

The review should prefer durable clarity over file-count progress. Deleting,
renaming, or converting markdown is not progress unless it follows from a
correct classification and preserves the current architecture.

## Ledger Shape

Every reconciled shard needs a ledger entry in the active runbook or `todo.md`
with this shape:

```text
Shard: src/backend/mir/aarch64/codegen/<name>.md
Classification: already-converted/reconcile-ledger | stale/reject-retire | real-missing-feature
Current Owner: <compiled files, idea 224 boundary, or none>
Review Result: <why the classification is correct>
Proof or Evidence: <existing selector, code reference, or reason no proof applies>
Follow-Up: <none, retire note, or ideas/open draft path to create>
```

## Expected Review Order

Start with shards that appear to overlap already-active compiled surfaces, then
move into stale structural evidence, then missing feature families:

- active compiled surfaces: `alu`, `comparison`, `returns`, `emit`
- current route boundaries and stale legacy evidence: `mod`, `records`,
  `asm_emitter`
- ABI and call/frame behavior: `calls`, `prologue`, `variadic`, `globals`
- scalar and wide operation families: `memory`, `cast_ops`, `float_ops`,
  `i128_ops`, `f128`
- target-specific extras: `atomics`, `intrinsics`, `inline_asm`, `peephole`

The exact order may change during planning, but each execution packet should
stay bounded to a small shard group and must not reopen active 224 work.

## Boundaries

- Do not treat this as a mandate to convert every markdown shard into C++.
- Do not resurrect legacy record-pile or broad module-emitter ownership.
- Do not convert `records.md` into `records.cpp` / `records.hpp` unless a
  later human-approved idea explicitly changes the architecture.
- Do not claim a missing-feature shard without first checking current compiled
  owners such as `alu.cpp`, `comparison.cpp`, `returns.cpp`, and `emit.cpp`.
- Do not move printer-boundary work out of idea 224 or add noise to active 224
  implementation state.
- Do not use AArch64 codegen as the owner for semantic facts that should be
  prepared by BIR or shared MIR layers.
- Do not downgrade backend cases, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add named-case shortcuts in C++ that only recognize a proof fixture.

## Completion Signal

This idea is complete when every listed codegen markdown shard has a recorded
classification, stale shards have explicit retire rationale, already-converted
shards map to current compiled owners, and every real missing-feature shard has
been turned into a focused follow-up idea or bounded implementation route with
a clear proof contract.

Completion does not require eliminating all markdown files or creating
same-named C++ files for every shard.

## Closure Note

Closed after the active runbook recorded exactly one primary classification for
all 20 listed source markdown shards. The final ledger inventory classified 5
shards as already-converted/reconcile-ledger, 3 shards as stale/reject-retire,
and 12 shards as real-missing-feature with focused follow-up ideas under
`ideas/open/231` through `ideas/open/240`; the full-scan follow-up remains in
`ideas/open/230`.

No implementation changes were required for closeout. Existing full-suite
regression logs passed the close guard under maintenance semantics:
`test_before.log` and `test_after.log` both recorded 3167 passing tests and 0
failures.

## Reviewer Reject Signals

Reject the route if it mechanically converts shards without architectural
review, recreates stale record-pile ownership, treats `records.md` as a mandate
for `records.cpp` / `records.hpp`, claims missing features without checking
current compiled owners, moves active printer-boundary work out of idea 224,
adds fixture-shaped special cases, weakens tests to appear green, or patches
missing prepared-BIR facts inside AArch64 codegen instead of splitting them
into a separate idea.
