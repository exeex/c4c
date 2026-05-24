Status: Active
Source Idea Path: ideas/open/entry-formal-publication-planning-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Prealloc Formal Publication Plan API

# Current Packet

## Just Finished

Step 2 added the prealloc-owned target-neutral formal-publication planning API.

`formal_publications.hpp/.cpp` now exposes
`plan_prepared_formal_publication(...)` and
`plan_prepared_formal_publications(...)`. The returned
`PreparedFormalPublicationPlan` preserves source Prepared facts only: formal
index, prepared value name/id, BIR type/formal pointer, `PreparedValueHome*`,
home kind, target-neutral action, and structured status.

The helper reports available incoming-register/incoming-stack publication
intent, no-publication formals, missing inputs, missing formal name, missing
value home, missing ABI info, missing register name, missing stack offset,
unsupported home kind, and unsupported formal source. It reuses prepared
value-home lookup inputs and does not compute target register names, target
stack offsets, MIR operands, inline asm, prologue behavior, diagnostics, or
copy emission.

Focused direct coverage was added in `backend_prealloc_formal_publications`
for register-home, stack-home, missing-home, missing stack-offset, missing
register-name, missing ABI, unsupported-home, no-publication, and collection
forms.

## Suggested Next

Step 3 should adapt AArch64 entry-formal dispatch to consume the prealloc
formal-publication plans while keeping ABI source selection, target register
spelling, stack offset policy, scalar-state recording, inline asm/copy
emission, prologue behavior, and diagnostics target-local.

## Watchouts

Keep ABI register and stack source policy, AArch64 register spelling, prologue
shape, and entry-copy instruction emission target-local. Do not combine this
with call-boundary classification, edge-copy bookkeeping, or operand decoding
migrations.

`dispatch_publication.cpp` block-entry move scans may be reusable later, but
they are not the first formal-publication extraction. Keep this packet scoped
to entry formal-to-home publication intent.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 154/154 backend tests. Proof log: `test_after.log`.
