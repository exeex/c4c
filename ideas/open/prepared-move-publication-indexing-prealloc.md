# Prepared Move Publication Indexing in Prealloc

## Intent

Move target-independent Prepared move and publication lookup indexing out of
`src/backend/mir/aarch64/codegen` and into `src/backend/prealloc` or an adjacent
read-only prepared-consumer helper.

## Why This Exists

The AArch64 backend currently owns construction and use of Prepared lookup
indexes for call plans, address materialization plans, move bundles, and
value-home records. Those indexes are not AArch64 instruction selection,
register spelling, ABI lane policy, or assembly printing. They are consumer
views over `PreparedBirModule` facts.

If x86 and later RISC-V consume Prepared lowering, duplicating these indexes in
each target would create parallel fact-recovery paths and make missing-fact
behavior drift by backend.

## Current AArch64-Owned Responsibility

Current responsibility lives in the AArch64 module/lowering context around
prepared-module traversal and dispatch helpers, especially the code paths that
build or query call-plan, address-materialization, move-bundle, and value-home
indexes before target emission.

## Proposed Destination Layer

`src/backend/prealloc`, or a tightly adjacent prepared-consumer support module
owned by prealloc, should expose read-only lookup helpers over prealloc-owned
Prepared facts.

The helper should not emit machine instructions. It should provide stable lookup
records that AArch64, x86, and later RISC-V can consume.

## x86/RISC-V Benefit

x86 already has prepared-plan consumption pressure around `ConsumedPlans` and
prepared lowering surfaces, so it can use the same read-only indexes instead of
reconstructing AArch64-style maps. RISC-V benefits later by receiving a defined
Prepared lookup contract when it moves beyond its current reference-style
text-emitter route.

## In Scope

- Identify the current AArch64 indexing sites and their input Prepared facts.
- Add prealloc-owned or prealloc-adjacent lookup helpers for the shared index
  shape.
- Convert AArch64 to consume the shared lookup helpers without changing target
  output.
- Add or update focused tests that prove unchanged AArch64 lowering for more
  than one function shape using moves/publications.
- Name the x86 consumption path, likely around existing prepared plan
  consumption such as `ConsumedPlans` and prepared lowering surfaces.

## Out of Scope

- Moving AArch64 instruction emission, register names, AAPCS64 policy, target
  addressing legality, or assembly printing.
- Rewriting call lowering or edge-copy emission behavior.
- Changing expectations to make narrow known failures pass.
- Introducing a generic target abstraction beyond the read-only lookup surface.

## Proof Needed To Avoid Testcase Overfit

Acceptance needs before/after proof that AArch64 output is unchanged for a small
set covering at least call-plan lookup, address-materialization lookup,
move-bundle lookup, and value-home lookup. The proof must include nearby
non-target cases, not only one failing testcase.

The implementation should also show that x86 can plausibly consume the helper
without copying the AArch64 context object or target policy.

## Reviewer Reject Signals

Reject the route if the diff adds named-case matching for one function, moves
AArch64 register or ABI policy into prealloc, weakens unsupported/supported test
contracts, or claims migration progress through helper renames while the actual
index construction remains target-local.

Reject if the new helper only mirrors AArch64 data structures with AArch64 names
and no clear x86 consumption path, or if the old missing-index failure mode is
kept behind a new abstraction name.
