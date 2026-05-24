Status: Active
Source Idea Path: ideas/open/entry-formal-publication-planning-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Adapt AArch64 To Consume Formal Publication Plans

# Current Packet

## Just Finished

Step 3 adapted AArch64 entry-formal dispatch to consume the prealloc formal
publication plans.

`lower_entry_formal_publications()` now calls
`prepare::plan_prepared_formal_publications(...)` and uses each
`PreparedFormalPublicationPlan` for the target-neutral formal/home iteration:
formal index, formal pointer, prepared value/home authority, and home kind.

AArch64 still owns incoming register vs stack source selection, overflow and
frame-size policy, source/destination register spelling, byval/f128 special
cases, scalar-state recording, inline asm/copy-line construction, prologue
behavior, diagnostics/status handling, and final instruction emission.

## Suggested Next

Step 4 should prove the reuse path for x86 entry lowering by exposing or using
the prealloc formal-publication plan from the x86 prepared/query surface,
without rewriting x86 prologue/lowering or moving x86 ABI operands into
prealloc.

## Watchouts

Keep ABI register and stack source policy, AArch64 register spelling, prologue
shape, and entry-copy instruction emission target-local. Do not combine this
with call-boundary classification, edge-copy bookkeeping, or operand decoding
migrations.

`dispatch_publication.cpp` block-entry move scans may be reusable later, but
they are not the first formal-publication extraction. Keep this packet scoped
to entry formal-to-home publication intent.

The prealloc plan's source action remains target-neutral. AArch64 still
rechecks its own incoming stack/register conditions because AAPCS64 overflow
and frame-size behavior are target policy.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 154/154 backend tests. Proof log: `test_after.log`.
