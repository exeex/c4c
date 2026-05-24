Status: Active
Source Idea Path: ideas/open/entry-formal-publication-planning-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Reuse Path For x86 Entry Lowering

# Current Packet

## Just Finished

Step 4 proved the x86 reuse path for the prealloc formal-publication plans.

`x86::prepared::Query` now exposes the prepared BIR function for its selected
function and provides `plan_formal_publication(...)` and
`plan_formal_publications()` wrappers over
`prepare::plan_prepared_formal_publication(s)`.

The existing x86 prepared internal test now constructs formal Prepared facts
and proves reuse across incoming-register, incoming-stack, missing-home, and
no-publication forms while preserving source formal/home facts and missing
authority behavior.

## Suggested Next

Step 5 should validate behavior and anti-overfit coverage for the active plan.

## Watchouts

The x86 packet proves the shared planning API is reusable from the prepared
query surface only. It does not rewrite x86 prologue/lowering, and x86 ABI
policy, source operands, register classes, operand spelling, and entry-copy
emission remain target-local.

`dispatch_publication.cpp` block-entry move scans may be reusable later, but
they are not the first formal-publication extraction. Keep this packet scoped
to entry formal-to-home publication intent.

The prealloc formal-publication plan remains fact/status-only: it does not
construct target operands, select x86 registers, or emit entry-copy code.

## Proof

Ran the delegated proof exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed, 154/154 backend tests. Proof log: `test_after.log`.
